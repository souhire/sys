#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> // type pid_t
#include <string.h>
#include <sys/wait.h> // wait pid
#include <errno.h>
#include <time.h>
#include <sys/sysctl.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h> 
#include <mach/mach_init.h>
#include <mach/mach_host.h>


#define NUM_MIN_OF_CORES 1
#define MAX_PRIME 100000

#include<mach/mach.h>


void do_primes()
{
  unsigned long i, num, primes = 0;
  for (num = 1; num <= MAX_PRIME; ++num) {
    for (i = 2; (i <= num) && (num % i != 0); ++i);
    if (i == num)
      ++primes;
  }
  printf("Calculated %lu primes.\n", primes);
}
// Parser proc line to get informations
int parseLine(char* line){
  int i = strlen(line);
  while (*line < '0' || *line > '9') line++;
  line[i-3] = '\0';
  i = atoi(line);
  return i;
}


static unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;
    
// Reading on proc stat
void init(){
  FILE* file = fopen("/proc/stat", "r");
  fscanf(file, "cpu %llu %llu %llu %llu", &lastTotalUser, &lastTotalUserLow,
	 &lastTotalSys, &lastTotalIdle);
  fclose(file);
}
    

// Get current value of proc stat.
double getCurrentValue(){
  double percent;
  FILE* file;
  unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;
    

  file = fopen("/proc/stat", "r");
  fscanf(file, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow,
	 &totalSys, &totalIdle);
  fclose(file);
    

  if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow ||
      totalSys < lastTotalSys || totalIdle < lastTotalIdle){
    //Overflow detection. Just skip this value.
    percent = -1.0;
  }
  else{
    total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
      (totalSys - lastTotalSys);
    percent = total;
    total += (totalIdle - lastTotalIdle);
    percent /= total;
    percent *= 100;
  }
    

  lastTotalUser = totalUser;
  lastTotalUserLow = totalUserLow;
  lastTotalSys = totalSys;
  lastTotalIdle = totalIdle;
    

  return percent;
}


/*
//About CPU:
static clock_t lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;
    

void init(){
  FILE* file;
  struct tms timeSample;
  char line[128];
    

  lastCPU = times(&timeSample);
  lastSysCPU = timeSample.tms_stime;
  lastUserCPU = timeSample.tms_utime;
    

  file = fopen("/proc/cpuinfo", "r");
  numProcessors = 0;
  while(fgets(line, 128, file) != NULL){
    if (strncmp(line, "processor", 9) == 0) numProcessors++;
  }
  fclose(file);
}
    

double getCurrentValue(){
  struct tms timeSample;
  clock_t now;
  double percent;
    

  now = times(&timeSample);
  if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
      timeSample.tms_utime < lastUserCPU){
    //Overflow detection. Just skip this value.
    percent = -1.0;
  }
  else{
    percent = (timeSample.tms_stime - lastSysCPU) +
      (timeSample.tms_utime - lastUserCPU);
    percent /= (now - lastCPU);
    percent /= numProcessors;
    percent *= 100;
  }
  lastCPU = now;
  lastSysCPU = timeSample.tms_stime;
  lastUserCPU = timeSample.tms_utime;
    

  return percent;
}

*/

/* Physical memory used */
int getValue(){ //Note: this value is in KB!
  FILE* file = fopen("/proc/self/status", "r");
  int result = -1;
  char line[128];
    

  while (fgets(line, 128, file) != NULL){
    if (strncmp(line, "VmRSS:", 6) == 0){
      result = parseLine(line);
      break;
    }
  }
  fclose(file);
  return result;
}
    
/* virtual memory on linux 

int getValue(){ //Note: this value is in KB!
  FILE* file = fopen("/proc/self/status", "r");
  int result = -1;
  char line[128];
    

  while (fgets(line, 128, file) != NULL){
    if (strncmp(line, "VmSize:", 7) == 0){
      result = parseLine(line);
      break;
    }
  }
  fclose(file);
  return result;
}
*/

int main(int ac, char** av)
{
  int requiredLoad	= 0;
  int timeToRich	= 0;
  int numberOfProc	= NUM_MIN_OF_CORES;
  if(1 < ac){ // if ac > 1 then 1 <  ac < 4  
    if(2 == ac){
      requiredLoad	= atoi(av[1]);
    }else{
      requiredLoad	= atoi(av[1]);
      timeToRich	= atoi(av[2]);
    }
  }
 
  time_t start, end;
  time_t run_time;
  unsigned long i;
  pid_t pids[numberOfProc];
  
  /* start of test */
  start = time(NULL);                                                                                                
  for (i = 0; i < numberOfProc; ++i) {
    if (!(pids[i] = fork())) {
      do_primes();
      exit(0);
    }
    if (pids[i] < 0) {
      perror("Fork");
      exit(1);
    }
  }
  for (i = 0; i < numberOfProc; ++i) {
    waitpid(pids[i], NULL, 0);
  }
  end = time(NULL);
  run_time = (end - start);
  printf("This machine calculated all prime numbers under %d %d times "
	 "in %lu seconds\n", MAX_PRIME, numberOfProc, run_time);

  // Information memory
struct task_basic_info t_info;
mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

if (KERN_SUCCESS != task_info(mach_task_self(),
                              TASK_BASIC_INFO, (task_info_t)&t_info, 
                              &t_info_count))
  {
    return -1;
  }
 printf("Residence usage %lu\n", t_info.resident_size);
 printf("Virtual size %lu\n", t_info.virtual_size);

 int mib[2];
 int64_t physical_memory;
 mib[0] = CTL_HW;
 mib[1] = HW_MEMSIZE;
 size_t length = sizeof(int64_t); 
 printf("Total physical memory : %d\n", sysctl(mib, 2, &physical_memory, &length, NULL, 0));

 // General informations

 vm_size_t page_size;
 mach_port_t mach_port;
 mach_msg_type_number_t count;
 vm_statistics_data_t vm_stats;
 int64_t mfm = 0;
 int64_t um = 0;
 mach_port = mach_host_self();
 count = sizeof(vm_stats) / sizeof(natural_t);
 if (KERN_SUCCESS == host_page_size(mach_port, &page_size) &&
     KERN_SUCCESS == host_statistics(mach_port, HOST_VM_INFO, 
				     (host_info_t)&vm_stats, &count))
   {
     int64_t mfm = (int64_t)vm_stats.free_count * (int64_t)page_size;

     int64_t um = ((int64_t)vm_stats.active_count + 
		    (int64_t)vm_stats.inactive_count + 
		    (int64_t)vm_stats.wire_count) *  (int64_t)page_size;
   }

 printf("Free Memory %lld \n", mfm);
 printf("Usage Memory %lld \n", um);
 init();
 // printf("Physical memory used: %d \n", getValue());

 
 // printf("Current use of my memory %f", getCurrentValue());
  return EXIT_SUCCESS;
}
