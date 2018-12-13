/**********************************************************************************
 *  Ryan Brazones - rbazones@gmail.com
 *
 *  File: metric.h
 *
 *  Description: Header file for metric testing functions used in the memory 
 *  coordinator project.
 *
 * ********************************************************************************/

#ifndef METRIC
#define METRIC

/*
 *  Compares memory usage to target, relative to a define threshold
 */

int compare(unsigned long long current, unsigned long long target, unsigned long long threshold);

/*
 *  Tests whether two inputs are equal to each other (within 5000)
 */

int equals(unsigned long long arg1, unsigned long long arg2);

/*
 *  Tests if arg1 is within the threshold relative to arg2
 */

int test_threshold(unsigned long long arg1, unsigned long long arg2, unsigned long long threshold);

#endif
