//this file reads the profile file and analyze the profile to find out the
//probability of a may-dependence for a loop
//it now performs a simple heuristics 
//if probability(dependent)>50% don't speculate
//else speculate
#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXLINE 100

 struct file_info
{
	int already_inserted; //stores the number of inserted pragmas in the source file
				  //we need this information because every time we insert a pragma,
				  //the line number to be inserted next changes
	char file_name[100];      //we also need to store the file name
	int line_numbers[1000]; 
	int number_of_lines;
};

 struct file_info_omp
{
	char file_name[100];      //we also need to store the file name
	int line_number;
};
struct file_info files[1000];
struct file_info_omp omp_struct[1000];
int number_of_files = 0;
int number_of_omp = 0;

void main(int argc, char *argv[])

{
	
	FILE *llvm_profile; //the llvm combined profile file
	FILE *omp; //the omp file
	FILE *loop_info;    //file which contains the file name(s) and line number(s) of all the loops
	FILE *read_file;       //temporaries to store the file name where pragma to be inserted
	FILE *write_file;
	
	char line [MAXLINE];
	//get omp file info
	omp = fopen("omp","r");
	if (omp == NULL) {
		printf("omp info file does not exist,exiting...\n");
		exit(1);
	}
	while (fgets (line, sizeof(line), omp) !=NULL)
	{
		//keep filling out the spec structures
		struct file_info_omp fi;
		sscanf(line, "%s %d",fi.file_name,&fi.line_number);
		omp_struct[number_of_omp++] = fi;
	}
	fclose(omp);

	llvm_profile = fopen("llvmprof.out","r");
	if (llvm_profile == NULL) {
		printf("Combined Profile file does not exist,exiting...\n");
		exit(1);
	}

	//iterate through the lines in the combined profile file
	//analyze each line, and modify the source files
	//by inserting pragmas accordingly
	int loop_id, execution_count, parallel, irregular, first_bin, second_bin, third_bin, fourth_bin, fifth_bin;

	while (fgets (line, sizeof(line), llvm_profile))
	{
		//printf("Read one line from llvm profile \n");
		sscanf(line, "%d %d %d %d %d %d %d %d\n", &loop_id, &parallel, &irregular, &first_bin, 
				&second_bin, &third_bin, &fourth_bin, &fifth_bin);
		if ((parallel < 0) && ((irregular < 0) || (first_bin < 0) || (second_bin < 0) || 
				(third_bin < 0) || (fourth_bin < 0) || (fifth_bin < 0)))
				//hard to analyse as there is no bias
				continue;
				
		execution_count = parallel + irregular + first_bin + second_bin + third_bin + fourth_bin + fifth_bin;
		float probability_independent = 0;
		if(execution_count!=0)
			probability_independent = ((parallel/ execution_count)*100);

		
		if ((probability_independent > 50) || ((parallel < 0) && ((irregular >= 0) && (first_bin >= 0) && 
				(second_bin >= 0) && (third_bin >= 0) && (fourth_bin >= 0) && (fifth_bin >= 0))) )
		{
			
			//we have to insert the pragma for this loop
			//we find the file name and line number
			int i = 0;
			char file[100];
			int line_number;
			char info_line[100];
			
			loop_info = fopen("loops","r");	
			if (loop_info == NULL) {
				printf("Loop Information file does not exist,exiting...\n");
				exit(1);
			}
			while(i != loop_id)
			{
			
				fgets (info_line, sizeof(info_line), loop_info);
				sscanf(info_line, "%s %d",file,&line_number);
				i++;
			}
			fclose(loop_info);
			//printf("Great prob for Loop id %d %s %d\n",loop_id,file,line_number);

			//we have got the file name and the line_number, now it's time to insert the pragma
 			//but first we have to check whether this is already parallelized by omp
 			
 			int z=0;
 			int has_omp = 0;
 			
 			for (z=0; z<number_of_omp; z++)
 			{
				if ((strcmp(omp_struct[z].file_name, file) == 0) && (omp_struct[z].line_number == line_number))
					has_omp = 1;
			}
			
			if(has_omp == 1) continue;
 			
 			
 			//now we have to check how many pragmas has already been
			//inserted into the file, as they will effect the line number of loops

			int flag = 0;    //flag to determine whether new file info has been added

			for(i=0; i<number_of_files; i++)
			{
				if(strcmp(files[i].file_name, file) == 0)
				{
					
					//we found the file, add the line number to it
					files[i].line_numbers[files[i].number_of_lines] = line_number;
					files[i].number_of_lines++;
					/*files[i].already_inserted++;
					//insert the pragma in the file
					char command[100] = "sed -i '";
					int line = line_number + files[i].already_inserted;
					char line1[15];
					sprintf(line1, "%d", line);
					strcat (command,line1);
					strcat (command," i #pragma speculative for' ");
					strcat (command, file);
					printf("%s\n",command);
					system (command);*/
					//make the flag = 1
					flag = 1;
					break;
					
				}
			}
			if(flag == 0)
			{
				//we didn't find the file in the previous for loop, we have to add that to our record
				
				strcpy (files[number_of_files].file_name,file);
				files[number_of_files].number_of_lines = 0;
				files[number_of_files].line_numbers[files[number_of_files].number_of_lines]=line_number;
				files[number_of_files].number_of_lines++;
				number_of_files++;
				/*//insert the pragma into the file
				char command[100] = "sed -i '";
				int line = line_number + files[i].already_inserted;
				char line1[15];
				sprintf(line1, "%d", line);
				strcat (command,line1);
				strcat (command," i #pragma speculative for' ");
				strcat (command, file);
				printf("%s\n",command);
				system (command);*/
			}
			
		}//end of if(probability>50)		
	}//end of iterating through llvmprof.out
	fclose(llvm_profile);	

	int x,y;
	for (x=0; x<number_of_files; x++)
	{
		printf("File name %s \n", files[x].file_name);
		
		//format the for( tokens in the file
		char temp[100] = "sed -i 's/for[ ]*(/for(/g' ";
		strcat(temp, files[x].file_name);
		//printf("%s \n",temp);
		system(temp);
		
		//rename the old file
		char temp_file_name[100];
		strcpy (temp_file_name,files[x].file_name);
		strcat (temp_file_name,"old");
		
		if(remove(temp_file_name) == 0)
		  printf("Old File %s  deleted.\n", temp_file_name);
		
		rename (files[x].file_name, temp_file_name);
		
		//open the old file in read mode and the new file in write mode
		read_file = fopen(temp_file_name,"r");
		if (llvm_profile == NULL) {
			printf("Old source file does not exist,exiting...\n");
			exit(1);
		}
		
		write_file = fopen(files[x].file_name,"w");
		if (llvm_profile == NULL) {
			printf("Failed to create new source file,exiting...\n");
			exit(1);
		}
		
		int line_no = 1;
		char temp_line [4000];
		//iterate through the lines of the old source file
		while(fgets (temp_line, sizeof(temp_line), read_file) !=NULL)
		{
			//if the current line number is to be instrumented
			int found = 0;
			for(y=0; y<files[x].number_of_lines; y++)
			{
				if (line_no == files[x].line_numbers[y])
				{
					found = 1;
					break;
				}
			}
			
			if(found == 0)
			{
				//we don't have to instrument the line
				fputs(temp_line, write_file);
			}
			else
			{
				
				//we have to instrument the file

				printf("Found for line number %d\n",line_no);
				//check if a speculation pragma was already inserted
				if(strcmp(temp_line, "#pragma speculative for") == 0)
					fputs(temp_line, write_file);
				else if (strstr (temp_line,"for(") != NULL)
				{
					//we got a fresh candidate for instrumentation
					//let's see if it is well formed
					char * token;
					
					
					char first_token [800];
					token = strstr (temp_line,"for(");
	
					char * first = temp_line;
	
					int i1 = 0;
					while(first != token)
					{
						first_token[i1] = *first;
						first++;
						i1++;
					}
					first_token[i1]='\0';
					
					//by this time, first_token contains the string before "for("
					//and *token is the rest of the string starting from "for("
					strcat(first_token,"\n#pragma speculative for\n");

					fputs(first_token,write_file);
					fputs(token, write_file);
					
					/*token = strtok (temp_line,"for(");
					int count1 = 1;
					while (token != NULL)
					{
						char temp_string_1[400];
						if(count1 == 1)
						{
							strcpy(temp_string_1,token);
							strcat(temp_string_1,"\n #pragma speculative for \n");
							fputs(temp_string_1,write_file);
							count1++;
						}
						else
						{
							//second token
							strcpy(temp_string_1,"for(");
							strcat(temp_string_1,token);
							fputs(temp_string_1,write_file);
						}
					}//end of while (token != NULL)*/
					
				}//end of else if
				else
				{
					//char temp_string_1[400];
					//fputs("#pragma speculative for \n",write_file);
					fputs(temp_line, write_file);
				}
			}//end of else (we instrumented)
			line_no++;
		}//end of iterating through the lines of old file
		
		fclose(read_file);
		fclose(write_file);
	}//end of for (x=0; x<number_of_files; x++)

}
