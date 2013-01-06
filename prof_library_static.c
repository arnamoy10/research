#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

//macros
#define MAX_ITERATIONS 50 //defines the number of iterations after which analyse() will be called
#define STRIDE_COUNT 5    //counter in the stride detection state table.
			  //if we find STRIDE_COUNT number of strides, we say it's a stride

#define MAXLINE 100            //defines how long a line in the profile file can be

#define MAX_LOOPS  5000      // stores the maximum number of loops that can be present in the benchmark
#define MAX_ACCESSES 2000    //the number of accesses that can be present for the loop sample
#define MAX_DEPENDENCES 2000 // maximum number of dependence_pairs for an execution of the loop sample

//structure declarations


//stores access information
struct Access

{
	int iteration_id;
	//unsigned int address;
	void * address;
};

//stores dependence pair
struct DependencePair
{
	int write;
	int read;
	char checked;
};


struct Loop_Info
{
	int loop_id;
	short parallel, irregular, first_bin, second_bin, third_bin, fourth_bin, fifth_bin;
	
};

//globals

struct Loop_Info loop_info[MAX_LOOPS];
struct Access    stores [MAX_LOOPS][MAX_ACCESSES];
struct DependencePair pairs [MAX_LOOPS] [MAX_DEPENDENCES];

int dep_pair_count[MAX_LOOPS];		//used to store the number of dependence_pairs detected for the run so that
					//we don't have to traqverse the whole array.
int access_count[MAX_LOOPS];		//same for the number of accesses


int initialize_flag = 0	;	//used to stop multiple initialization of data structures
int loop_info_count = 0;	//used to keep track of how many loop_info are stored
int flag_in_library[MAX_LOOPS] = {0};
int stride_in_library[MAX_LOOPS] = {0};
int loop_trip_count =0; 	//variable to store max trip_count we care about in case of early exit

//unsigned int loops[MAX_LOOP_COUNT] = {0}; //used to track how many dependence pairs per loop
char output[16];//for writing to files


//function declarations
int print_load_int8(char * x,int count,int loop);
int print_store_int8(char * x,int count,int loop);
int print_load_int16(short * x,int count,int loop);
int print_store_int16(short * x,int count,int loop);
int print_load_int32(int * x,int count,int loop);
int print_store_int32(int * x,int count,int loop);
int print_load_int64(long int * x,int count,int loop);
int print_store_int64(long int * x,int count,int loop);
int print_load_float(float * x,int count,int loop);
int print_store_float(float * x,int count,int loop);
int print_load_double(double * x,int count,int loop);
int print_store_double(double * x,int count,int loop);
int print_load_long_double(long double * x,int count,int loop);
int print_store_long_double(long double * x,int count,int loop);

void common(void * ,int ,int ,char);


void initialize();
void add_store(int, int, void *);
void check_dependence(int, int, void *);
int analyse_and_write(int loop);
void print(int);
int calculate_stride(int);
int write_to_file();
void make_discovered(int read, int write, int stride, int loop, int start);
int check_stride(int write,int stride,int loop, int start_point);
int search_in_list(int write, int read, int loop, int start);
void write_to_LoopInfo(int loop,short independent,int stride);

int print_load_pointer(void * x,int count,int loop)
{
	//printf("%d %d l %p \n",loop, count, x);
	common(x,count,loop,'l');
	return 1;
}

int print_store_pointer(void * x,int count,int loop)
{
	//printf("%d %d l %p \n",loop, count, x);
	common(x,count,loop,'s');
	return 1;
}

int print_load_int8(char * x,int count,int loop)
{
	//printf("%d %d l %p \n",loop, count, x);
	common(x,count,loop,'l');
	return 1;
}
int print_load_int16(short * x,int count,int loop)
{
	//printf("%d %d l %p \n",loop, count, x);
	common(x,count,loop,'l');
	return 1;
}
int print_load_int32(int * x,int count,int loop)
{
	//printf("%d %d l %p \n",loop, count, x);
	common(x,count,loop,'l');
	return 1;
}

int print_load_int64(long int * x,int count,int loop)
{
	//printf("%d %d l %p \n",loop, count, x);
	common(x,count,loop,'l');
	return 1;
}

//floats
int print_load_float(float * x,int count,int loop)
{
	//printf("%d %d l %p \n",loop, count, x);
	common(x,count,loop,'l');
	return 1;
}
int print_load_double(double * x,int count,int loop)
{
	//printf("%d %d l %p \n",loop, count, x);
	common(x,count,loop,'l');
	return 1;
}
int print_load_long_double(long double * x,int count,int loop)
{
	//printf("%d %d l %p \n",loop, count, x);
	common(x,count,loop,'l');
	return 1;
}

//stores

int print_store_int8(char * x,int count,int loop)
{
	//printf("%d %d s %p \n",loop, count, x);
	common(x,count,loop,'s');
	return 1;
}
int print_store_int16(short * x,int count,int loop)
{
	//printf("%d %d s %p \n",loop, count, x);
	common(x,count,loop,'s');
	return 1;
}
int print_store_int32(int * x,int count,int loop)
{
	//printf("%d %d s %p \n",loop, count, x);
	common(x,count,loop,'s');
	return 1;
}
int print_store_int64(long int * x,int count,int loop)
{
	//printf("%d %d s %p \n",loop, count, x);
	common(x,count,loop,'s');
	return 1;
}

//floats
int print_store_float(float * x,int count,int loop)
{
	//printf("%d %d s %p \n",loop, count, x);
	common(x,count,loop,'s');
	return 1;
}
int print_store_double(double * x,int count,int loop)
{
	//printf("%d %d s %p \n",loop, count, x);
	common(x,count,loop,'s');
	return 1;
}
int print_store_long_double(long double * x,int count,int loop)
{
	//printf("%d %d s %p \n",loop, count, x);
	common(x,count,loop,'s');
	return 1;
}

void common(void * x,int count,int loop, char type)
{
	if(type == 'l')
	{
		if (count <= MAX_ITERATIONS)
		{	
			//printf("less than max iterations load \n");
			//initialize the data structures for the first time
			if(initialize_flag == 0)
			{
				initialize();
				initialize_flag = 1;
			}
			check_dependence(loop, count, x);
		}
		else if(count == (MAX_ITERATIONS+1) && (flag_in_library[loop-1] == 0))
		{
			//printf("greater than max iterations load \n");
			//print(loop);
	
			if(dep_pair_count[loop-1] == 0)
			{
				write_to_LoopInfo(loop,1,0);//printf("Independent Loop %d\n",loop);
	
			}
			else
			{
				int stride = calculate_stride(loop);
				if(stride == 0)
				{
					write_to_LoopInfo(loop,0,0);
					//printf("Irregular for Loop %d\n",loop);
				}
				else
				{
					write_to_LoopInfo(loop,0,stride_in_library[loop-1]);
					//printf("Stride %d for Loop %d\n",stride_in_library[loop-1],loop);
				}
			}
			
			flag_in_library[loop - 1] = 1;
			access_count[loop - 1] = 0;
			dep_pair_count[loop - 1] = 0;
		}
	}
	else
	{
		if (count <= MAX_ITERATIONS)
		{
			//initialize the data structures for the first time
			//printf("less than max iterations store \n");
			if(initialize_flag == 0)
			{
				initialize();
				initialize_flag = 1;
			}
			add_store(loop, count, x);
		}
		else if(count == (MAX_ITERATIONS+1) && (flag_in_library[loop - 1] == 0))
		{
			//printf("greater than max iterations store\n");
			//print(loop);
			if(dep_pair_count[loop-1] == 0)
			{
				write_to_LoopInfo(loop,1,0);//printf("Independent Loop %d\n",loop);
	
			}
			else
			{
				int stride = calculate_stride(loop);
				if(stride == 0)
				{
					write_to_LoopInfo(loop,0,0);
					//printf("Irregular for Loop %d\n",loop);
				}
				else
				{
					write_to_LoopInfo(loop,0,stride_in_library[loop-1]);
					//printf("Stride %d for Loop %d\n",stride_in_library[loop-1],loop);
				}
			}
			flag_in_library[loop - 1] = 1;
			access_count[loop - 1] = 0;
			dep_pair_count[loop - 1] = 0;
		}
	}
}

void initialize()
{
	//printf("Inside Initialize \n");
	int i = 0, j = 0;
	while(i<MAX_LOOPS)
	{
		/*struct Loop_Info a;
		a.loop_id = -1;
		a.parallel = 0;
		a.irregular = 0;
		a.first_bin = 0;
		a.second_bin = 0;
		a.third_bin = 0;
		a.fourth_bin= 0;
		a.fifth_bin = 0;
		loop_info[i] = a;*/
		flag_in_library[i] = 0;
		stride_in_library[i] = 0;
		dep_pair_count[i] = 0;
		access_count[i] = 1;
		i++;
	}
	i=0;
	while(i<MAX_LOOPS)
	{
		j=0;
		while(j<MAX_ACCESSES)
		{
			struct Access a;
			//a.iteration_id = 234;
			//a.address = (void *)0x23456576;
			stores[i][j] = a;
			j++;
		}
		i++;
	}
	i=0;
	while(i<MAX_LOOPS)
	{
		j=0;
		while(j<MAX_DEPENDENCES)
		{
			struct DependencePair a;
			//a.write = 234;
			//a.read = 112;
			//a.checked = 'l';
			pairs[i][j] = a;
			j++;
		}
		i++;
	}
	//printf("Done initializing \n");
	return;
}

void add_store(int loop, int iteration, void *addr)
{
	//checking for overflow condition
	if (access_count[loop-1] < (MAX_ACCESSES -2))
	{
		stores[loop-1][access_count[loop-1]].iteration_id = iteration;
		stores[loop-1][access_count[loop-1]].address = addr;
		access_count[loop-1]++;
	}
}

void check_dependence(int loop, int iteration, void *addr)
{
	//check for overflow condition
	if(dep_pair_count[loop-1] < (MAX_DEPENDENCES -2))
	{
		short i = 0;
		short accesses = access_count[loop-1];
		//printf("Accesses")
		while(i < accesses)
		{
			if ((addr == stores[loop-1][i].address) && (iteration > stores[loop-1][i].iteration_id))
			{
				//we got a dependence pair for the loop
				pairs[loop-1][dep_pair_count[loop-1]].write = stores[loop-1][i].iteration_id;
				pairs[loop-1][dep_pair_count[loop-1]].read = iteration;
				pairs[loop-1][dep_pair_count[loop-1]].checked = 'n';
				dep_pair_count[loop-1]++;
			}
			i++;
		}
	}
}

int analyse_and_write(int loop)
{
	if(flag_in_library[loop -1] == 1)
		flag_in_library[loop -1] = 0;
	else
	{
		//print(loop);
		if(dep_pair_count[loop-1] == 0)
		{
			write_to_LoopInfo(loop,1,0);//printf("Independent Loop %d\n",loop);

		}
		else
		{
			int stride = calculate_stride(loop);
			if(stride == 0)
			{
				write_to_LoopInfo(loop,0,0);
				//printf("Irregular for Loop %d\n",loop);
			}
			else
			{
				write_to_LoopInfo(loop,0,stride_in_library[loop-1]);
				//printf("Stride %d for Loop %d\n",stride_in_library[loop-1],loop);
			}
		}
		access_count[loop-1] = 0;
		dep_pair_count[loop-1] = 0;
	}
	return 1;
}

void write_to_LoopInfo(int loop,short independent,int stride)
{
	//try to see if the loop information is already there
	
	int i = 0;
	for(int i = 0; i < loop_info_count; i++)
	{
		if(loop_info[i].loop_id == loop)
		{
			if(independent) loop_info[i].parallel++;
			else
			{
				//we got either strided or irregular pattern
				if(stride == 0) loop_info[i].irregular++;
				else
				{
					if((stride >= 1) && (stride <=20))
						loop_info[i].first_bin++;
					if((stride >= 21) && (stride <=40))
						loop_info[i].second_bin++;
					if((stride >= 41) && (stride <=60))
						loop_info[i].third_bin++;
					if((stride >= 61) && (stride <=80))
						loop_info[i].fourth_bin++;
					if((stride >= 81) && (stride <=100))
						loop_info[i].fifth_bin++;
				}
			}
			return;
		}
	} 
	//we got here because we did not find any existing lop info
	//we have to create a new one
	struct Loop_Info li;
	li.loop_id = loop;

	if(independent) 
	{
		li.parallel = 1;
		li.irregular = li.first_bin = li.second_bin = li.third_bin = li.fourth_bin = li.fifth_bin = 0;
	}
	else
	{
		//we got either strided or irregular pattern
		if(stride == 0) 
		{
			li.irregular = 1;
			li.first_bin = li.parallel = li.second_bin = li.third_bin = li.fourth_bin = li.fifth_bin = 0;
		}
		else
		{
			if((stride >= 1) && (stride <=20))
			{
				li.first_bin = 1;
				li.irregular = li.parallel = li.second_bin = li.third_bin = li.fourth_bin = li.fifth_bin = 0;
			}
			if((stride >= 21) && (stride <=40))
			{
				li.second_bin = 1;
				li.irregular = li.parallel = li.first_bin = li.third_bin = li.fourth_bin = li.fifth_bin = 0;
			}
			if((stride >= 41) && (stride <=60))
			{
				li.third_bin = 1;
				li.irregular = li.parallel = li.second_bin = li.first_bin = li.fourth_bin = li.fifth_bin = 0;
			}
			if((stride >= 61) && (stride <=80))
			{
				li.fourth_bin = 1;
				li.irregular = li.parallel = li.second_bin = li.third_bin = li.first_bin = li.fifth_bin = 0;
			}
			if((stride >= 81) && (stride <=100))
			{
				li.fifth_bin = 1;
				li.irregular = li.parallel = li.second_bin = li.third_bin = li.fourth_bin = li.first_bin = 0;
			}
		}
	}
	loop_info[loop_info_count] = li;
	loop_info_count++;


    return;
}


int write_to_file()
{

	printf("Printing loop-info count \n");
	int i = 0;
	int count = loop_info_count;
	for ( i = 0; i < count; i++)
	{
		printf("%d %d %d %d %d %d %d %d\n",loop_info[i].loop_id, loop_info[i].parallel, 
			loop_info[i].irregular, loop_info[i].first_bin, loop_info[i].second_bin, loop_info[i].third_bin, 
			loop_info[i].fourth_bin, loop_info[i].fifth_bin);
		loop_info_count --;
	}
	return 1;
}


int calculate_stride(int loop)
{
	int i = 0;
	int count = dep_pair_count[loop - 1];
	while (i < count)
	{
		if(pairs[loop-1][i].checked == 'n')
		{
			int read = pairs[loop-1][i].read;
			int write = pairs[loop-1][i].write;
			//printf("Checking for %d -%d\n",write, read);
			int stride = read - write;
			int got_stride = check_stride(write,stride,loop,i);

			if(got_stride == 1)
			{
				make_discovered(read,write,stride,loop,i);
			}
			if((got_stride == 1) && (stride_in_library[loop -1] == 0))
			{
					//printf("Got stride for first time \n");
					stride_in_library[loop-1] = stride;
					
			}
			if((got_stride == 1) && (stride <= stride_in_library[loop -1]))
			{
				//printf("Got stride for later \n");
				stride_in_library[loop-1] = stride;
				
			}
			if(got_stride == 0)
			{
				make_discovered(read,write,0,loop,i);
				return 0;
			}
		}
		i++;
	}
	return 1;
}

void make_discovered(int read, int write, int stride, int loop, int start)
{
	int i = start;
	int no_of_dep = dep_pair_count[loop-1];
	if(stride == 0)
	{
		while (i<no_of_dep)
		{	
			if((pairs[loop-1][i].read == read) && (pairs[loop-1][i].write == write))
			{
				pairs[loop-1][i].checked = 'y';
				return;
			}
			i++;
		}
	}
	else
	{
		//we have to delete a bunch of dependence pairs
		while (i<no_of_dep)
		{	
			int temp_stride = pairs[loop-1][i].read - pairs[loop-1][i].write;
			if(temp_stride == stride)
			{
				//printf("Making discovered for %d-%d\n",pairs[loop-1][i].write,pairs[loop-1][i].read);
				pairs[loop-1][i].checked = 'y';
			}
			i++;
		}
	}
}

int check_stride(int write,int stride,int loop, int start_point)
{
	int stride_number_count = 0;
	int i = 0;
	int w = (write+1);
	int r = (write+1+stride);
	while(i<5)
	{
		//printf("searching for %d %d\n",w,r);
		int found = search_in_list(w, r,loop, start_point);
		if(found == 1)
		{
			stride_number_count++;
			w++;
			r++;
		}
		else
			break;
		i++;
	}
	//printf("Value of stride_no_count %d\n",stride_number_count);
	if(stride_number_count == STRIDE_COUNT)
	{
		//we got a stride
		return 1;
	}
	else
	{
		//we did not get a stride so we delete only the dependence pair
		//that was passed to us
		//del_dependence_pair(write,(write+stride),loop);
		return 0;
	}
	
}

int search_in_list(int write, int read, int loop, int start)
{
	int i = start;
	while(i < dep_pair_count[loop - 1])
	{
		if((pairs[loop -1][i].write == write) && (pairs[loop -1][i].read == read))
			return 1;
		i++;
	}
	return 0;
}
void print(int loop)
{
	printf("Inside print \n");
	int i = 0, j = 0;
	while(i < dep_pair_count[loop-1])
	{
		printf("%d -> %d\n",pairs[loop -1][i].write,pairs[loop -1][i].read);
		i++;
	}
}
