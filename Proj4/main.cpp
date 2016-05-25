#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <cmath>

const float GRAIN_GROWS_PER_MONTH = 12.5;
const float ONE_DEER_EATS_PER_MONTH = 0.7;

const float ONE_BEAR_EATS_GRAINS_PER_MONTH = 0.3;
const float ONE_BEAR_EATS_DEERS_PER_MONTH = 0.7;

const float AVG_PRECIP_PER_MONTH = 3.0;
const float AMP_PRECIP_PER_MONTH = 3.0;
const float RANDOM_PRECIP = 2.0;

const float AVG_TEMP = 50.0;
const float AMP_TEMP = 22.0;
const float RANDOM_TEMP = 8.0;

int	NowYear;		// 2016 - 2021
int	NowMonth;		// 0 - 11
int NowMonthFake;

float	NowPrecip;		// inches of rain per month
float	NowTemp;		// temperature this month
float	NowHeight;		// grain height in inches
int		NowNumDeer;		// number of deer in the current population
int		NowNumBear;		// number of bears in the current population

float Ranf(float low, float high) {
	float r = (float)rand();               // 0 - RAND_MAX

	return(low + r * (high - low) / (float)RAND_MAX);
}

int Ranf(int ilow, int ihigh) {
	float low = (float)ilow;
	float high = (float)ihigh + 0.9999f;

	return (int)(Ranf(low, high));
}

void printState() {

	float tempTemp = (5.0 / 9.0)*(NowTemp - 32);
	float tempPrecip = NowPrecip*2.54;

	if (NowMonth == 0) {
		printf("Year = %d\n", NowYear);
		printf("\tMonth\tTemp\tPrecip\tGrains\tDeers\tBears\n");
	}

	printf("\t%d", NowMonthFake);
	printf("\t%.2lf", tempTemp);
	printf("\t%.2lf", tempPrecip);
	printf("\t%.2lf", NowHeight);
	printf("\t%d", NowNumDeer);
	printf("\t%d\n", NowNumBear);

}

void setTempPrecip() {
	float ang = (30.*(float)NowMonth + 15.) * (M_PI / 180.);

	float temp = AVG_TEMP - AMP_TEMP * cos(ang);
	NowTemp = temp + Ranf(-RANDOM_TEMP, RANDOM_TEMP);

	float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin(ang);
	NowPrecip = precip + Ranf(-RANDOM_PRECIP, RANDOM_PRECIP);
	if (NowPrecip < 0.) {
		NowPrecip = 0.;
	}
}

float getGrainHeight() {
	float tempHeight = NowHeight;

	float tempFactor = expf(-pow((NowTemp - AVG_TEMP) / 10, 2));
	float precipFactor = expf(-pow((NowPrecip - AVG_PRECIP_PER_MONTH) / 10, 2));

	tempHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
	tempHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;

	// Based on how many bears there are, decrease the height 
	tempHeight -= (float)NowNumBear * ONE_BEAR_EATS_GRAINS_PER_MONTH;

	if (tempHeight < 0.0) {
		tempHeight = 0.0;
	}

	return tempHeight;
}

void Deer() {
	// compute a temporary next-value for this quantity based on the current state of the simulation:
	float tempHeight = getGrainHeight();				// Tells us how much grain we have to work with
	int tempDeer = NowNumDeer;							// Use to tell how much deers are consuming
	int tempBear = NowNumBear;

	float needGrainHeight = tempDeer * ONE_DEER_EATS_PER_MONTH;
	
	float grainDeerRat = tempHeight / needGrainHeight;

	if (grainDeerRat > 2.0) {
		tempDeer += 3;
	}
	else if (grainDeerRat > 1.0) {
		tempDeer += 2;
	}
	else if (grainDeerRat < 1) {
		tempDeer -= 1;
	}

	// TODO Check bear count and change tempDeer count
	float needNumBearEat = tempBear * ONE_BEAR_EATS_DEERS_PER_MONTH;
	tempDeer -= (int)needNumBearEat;

	if (tempDeer <= 0) {
		tempDeer = 1;
	}

	// DoneComputing barrier:
	#pragma omp barrier
	NowNumDeer = tempDeer;

	// DoneAssigning barrier:
	#pragma omp barrier

	// DonePrinting barrier:
	#pragma omp barrier
}

void Grain() {
	// compute a temporary next-value for this quantity based on the current state of the simulation:
	int tempHeight = getGrainHeight();

	// DoneComputing barrier:
	#pragma omp barrier
	NowHeight = tempHeight;

	// DoneAssigning barrier:
	#pragma omp barrier

	// DonePrinting barrier:
	#pragma omp barrier
}

void Watcher() {
	// compute a temporary next-value for this quantity based on the current state of the simulation:

	// DoneComputing barrier:
	#pragma omp barrier
	// DoneAssigning barrier:
	#pragma omp barrier

	printState();			// Print stuff

	// Incrememnt month
	if (NowMonth == 11) {
		NowMonth = 0;
		NowMonthFake++;
		NowYear++;			//	Year also needs to be incremented
	}
	else {
		NowMonth++;			// It's not Dec, so only month needs to be incremented
		NowMonthFake++;
	}

	setTempPrecip();

	// DonePrinting barrier:
	#pragma omp barrier

}

/** Idea is that bears eat both deers and grains.
 *	Deer population is directly affected by the bear population.
 *	More bears when there are more deers.
 *	Grain height will also be affected.
 *	Ratio of bears eating deer-to-grain is more.*/
void Bears() {
	// Height and Deer count needed because bears are affected by both.

		// compute a temporary next-value for this quantity based on the current state of the simulation:
	float tempHeight = getGrainHeight();
	int tempDeer = NowNumDeer;
	int tempBear = NowNumBear;

	//float
	// TODO How many deers exist? Increase Bear count accordingly
	float DeerBearRatio = tempDeer / tempBear;

	if (DeerBearRatio > 2) {
		tempBear += 2;
	}
	else if (DeerBearRatio > 1) {
		tempBear += 1;
	}
	else if (DeerBearRatio < 0.5) {
		// Only case where Bear count goes down
		tempBear -= 2;
	}

	if (tempBear <= 0) {
		tempBear = 1;
	}

	// DoneComputing barrier:
	#pragma omp barrier
	NowNumBear = tempBear;

	// DoneAssigning barrier:
	#pragma omp barrier

	// DonePrinting barrier:
	#pragma omp barrier

}

int main(int argc, char *argv[]) {

#ifndef _OPENMP
	fprintf(stderr, "OpenMP is not available\n");
	return 1;
#endif

	// Starting date and time:
	NowMonth = 0;
	NowMonthFake = 0;
	NowYear = 2016;

	// Starting state (feel free to change this if you want):
	NowHeight = 3.;
	NowNumDeer = 4;
	NowNumBear = 1;
	setTempPrecip();


	omp_set_num_threads(4);
	while (NowYear <= 2021) {
		#pragma omp parallel sections
		{

			#pragma omp section
			{
				Deer();
			}

			#pragma omp section
			{
				Grain();
			}

			#pragma omp section
			{
				Watcher();
			}

			#pragma omp section
			{
				Bears();
			}
		}
	}		// Implied barrier - all functions must return in order to allow any of them to get past here

	return 0;
}