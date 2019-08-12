/*
A Project of Andrea Cioni

Title:     Fan Daemon (FanD)

Author:    Andrea Cioni (andreacioni)
           cioni95@gmail.com

Copyright: 	
	Copyright (c) 2017 Andrea Cioni <cioni95@gmail.com>
	https://github.com/dweeber/WiFi_Check

Purpose:
	Simple utility program for GNU/Linux systems to control a fan motor of your miniPC (UDOO,Raspberry Pi,Orange Pi,etc) through a PWM signal.

Compile:
	gcc -std=gnu99 fand.c -o fand

Launch:
	fand <frequency of the PWM signal> <sleep seconds between temperature checks> <min temp in Celsius> <max temp in Celsius> <min dc percent> <max dc percent>
			
Example usage: 
	./fand 2 3 20 30 10 100

*/

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>

#define DEBUG 1
#define HELLO_BANNER "Fan Daemon"
#define VERSION "(0.3v)"
#define BASE_PATH "/sys/class/pwm/pwmchip0/"
#define EXPORT_PATH BASE_PATH "pwm0/"

#if DEBUG
#define DBG_RET(_ret) printf("%s\n", dbg_txt[_ret])
#else
#define DGB_RET(_ret)
#endif

typedef union
{
    struct
    {
        uint32_t frequence;
        uint32_t sleep_timeout;
        uint32_t min_temp;
        uint32_t max_temp;
        uint32_t min_percent_dc;
        uint32_t max_percent_dc;
        uint32_t period; /* Not as an argument */
    }val;
    uint32_t word[7];
}fandArguments_T;

typedef enum
{
    OK,
    ERR_EXPORT,
    ERR_UNEXPORT,
    ERR_ENABLE,
    ERR_DISABLE,
    ERR_SETUP,
    ERR_DUTY,
    NOK
}FAND_STATE_E;

fandArguments_T console_args;

static char* dbg_txt[] = {
    "OK",
    "ERR_EXPORT",
    "ERR_UNEXPORT",
    "ERR_ENABLE",
    "ERR_DISABLE",
    "ERR_SETUP",
    "ERR_DUTY",
    "NOK"
};


static volatile bool go = true;

int export(void);
int unexport(void);
int enable(bool enable);
int setup(uint32_t period);
int set_duty(uint8_t percent, uint64_t period);
float read_cpu_temp(void);
void kill_handler(int sig);
int run(fandArguments_T* _args);
bool parse_arguments(int argv, char **argc, fandArguments_T *_args);
int main(int argv, char **argc);


int export(void)
{
    int ret = ERR_EXPORT;

    printf("Exporting...");
    if (0u == system("echo 0 > " BASE_PATH "export"))
    {
        ret = OK;
    }

    DBG_RET(ret);

	return ret;
}

int unexport(void)
{
    int ret = ERR_UNEXPORT;
    
    printf("Unexporting...");
    if(0u == system("echo 0 > " BASE_PATH "unexport"))
    {
        ret = OK;
    }

    DBG_RET(ret); 

    return ret;
}

int enable(bool enable)
{
    int ret = ERR_ENABLE;

	if(enable)
	{
        printf("Enabling...");
        if(0 == system("echo 1 > " EXPORT_PATH "enable"))
        {
            ret = OK;
        }
	}
	else
	{
        printf("Disabling...");
        if(0 == system("echo 0 > " EXPORT_PATH "enable"))
        {
            ret = OK;
        }
	}

    DBG_RET(ret);

    return ret;
}

int setup(uint32_t period)
{
    int ret = ERR_SETUP;
	char buf[1024];

    printf("Setupping... %u s -> ", period);
    if((sprintf(buf, "echo %u > " EXPORT_PATH "period", period) != 0) \
        && system("echo normal > " EXPORT_PATH "polarity") == 0 && (system(buf) == 0))
    {
        ret = OK;
    }

    DBG_RET(ret);

    return ret;
}

int set_duty(uint8_t percent, uint64_t period)
{
    int ret = ERR_DUTY;
	char buf[1024];
	char processname[256];
	uint64_t duty;

	percent = (percent > 100) ? 100 : percent;
	duty = ((uint64_t)percent * period / 100);
	duty = (duty >= period) ? (period-1) : duty;
	
	printf("Percent: %u%%, DC: %lu/%lu\n", percent, duty, period);
	sprintf(processname,"FanD(DC %u%% %lu/%lu)", percent, duty, period);
	prctl(PR_SET_NAME, processname, NULL, NULL, NULL);

    if((sprintf(buf,"echo %lu > " EXPORT_PATH "duty_cycle",duty) != 0) && (system(buf) == 0))
    {
        ret = OK;
    }

    DBG_RET(ret);

	return ret;
}

float read_cpu_temp(void) 
{
	float ret = -1.0f;
	FILE *fp = popen("/bin/cat /sys/devices/virtual/thermal/thermal_zone0/temp", "r");
	char buf[1024] = {0};
	
	if(NULL != fp)
	{
		if(fgets(buf, 1023, fp) != NULL) 
        {
			printf("Temperature read: %s", buf);
			
			if((ret = strtoul(buf,NULL,10)) != 0)
            {
				ret = ret/1000;
			}
            else
            {
				printf("Cannot convert output string\n");
			}
		}
        else 
        {
			printf("Cannot read CPU temperature from output\n");
		}
		pclose(fp);	
	} 
    else 
    {
		printf("Cannot read CPU temperature\n");
	}
		
	int temp = (int)(ret < 0 ? (ret - 0.5) : (ret + 0.5));
	printf("%d\n", temp);
	printf("%f\n", ret);

	return ret;
}

void kill_handler(int sig)
{
    (void)sig;
	printf("Terminating...\n");
	go = false;
}

int run(fandArguments_T* _args)
{
	int ret = 0;
	int rotating = 0;
    float cpu_temp = 0;
    float ratio = 0;
    float percentdc = 0;
	signal(SIGINT, kill_handler);
	
	while(go) 
    {
		cpu_temp = read_cpu_temp();

		if(cpu_temp >= _args->val.min_temp) 
        {
			ratio = (cpu_temp - (float)_args->val.min_temp)/(float)(_args->val.max_temp - _args->val.min_temp);
			printf("%f\n", ratio);
			percentdc = (ratio * (float)(_args->val.max_percent_dc - _args->val.min_percent_dc))+ (float)_args->val.min_percent_dc;

			/*Kickstart the fan at 100% for 0->1 seconds if not rotating*/
			if(rotating == 0)
            {
				set_duty(100, _args->val.period);
				usleep(100000);
			}
			rotating = 1;
		} 
        else 
        {
			rotating = 0;
		}

		if(OK != set_duty(percentdc,_args->val.period))
        {
			printf("There was an error setting the duty cicle");
        }
		
		sleep(_args->val.sleep_timeout);
		
		printf("\n");
	}
	
	return ret;
}

bool parse_arguments(int argv, char **argc, fandArguments_T *_args)
{
	bool ret = OK;
    uint64_t ul; 

	if(7 == argv)
	{

        for(uint8_t i = 0; i < 6; i++)
        {
		    ul = strtoul(argc[i+1],NULL,10);
            if(0 != ul)
            {
                _args->word[i] = ul; 
            }
            else
            {
                ret = NOK;
                break;
            }
        }

        if(OK == ret)
        {
			printf("Frequency: %u Hz\n", _args->val.frequence);
            _args->val.period = 100000000 / _args->val.frequence;
            printf("Period: %u s\n", _args->val.period);
			printf("Sleep timeout: %u s\n",_args->val.sleep_timeout);
			printf("Min temperature: %u *C\n",_args->val.min_temp);
			printf("Max temperature: %u *C\n",_args->val.max_temp);
			printf("Min duty cycle: %i %%\n",_args->val.min_percent_dc);
			printf("Max duty cycle: %i %%\n",_args->val.max_percent_dc);
		}
			
	} 
    else
    {
        ret = NOK;
    }

	return ret;
}

int main(int argv, char **argc)
{
	int ret = 0;
    fandArguments_T _args;

	printf("%s %s\n",HELLO_BANNER,VERSION);
	
	if(OK != parse_arguments(argv, argc, &_args))
	{
		printf("fand <frequency> <sleep seconds> <min temp> <max temp> <min dc percent> <max dc percent> \n");
		ret = 100;
	}
	else
	{
		if(OK == export()) 
        {
			/*Reset Duty Cycle before applying period*/
			set_duty(1,1);
			
			if(OK == setup(_args.val.period))
			{
				if(OK == enable(true)) 
                {
					ret = run(&_args);
					enable(false);  /* Disable */
				} 
			}
		} 

		unexport();
	}
	
	return ret;
}

