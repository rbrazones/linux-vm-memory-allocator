/**********************************************************************************
 *  Ryan Brazones - rbazones@gmail.com
 *
 *  File: metric.c
 *
 *  Description: Contains implementations of the fuctions given in metric.h
 *
 * ********************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "metric.h"

int compare(unsigned long long current, unsigned long long target, unsigned long long threshold){

    if (equals(current, target) == 1){
       
        /*
         *  Do nothing
         */
       
        return 0;

    } else if (current > target){

        if (test_threshold(current, target, threshold) == 1){

            /*
             *  Minus threshold from memory
             */

            return 1;
        }

        else{
        
            /*
             *  Minus the difference
             */

            return 2;
        }

    }else if (current < target){

        if (test_threshold(current, target, threshold) == 1){
            
            /*
             *  Add 2x threshold
             */

            return 3;
        }

        else{

            /*
             *  Add 1x threshold
             */

            return 4;
        }

    } else {

        /*
         *  Undefined condition = error
         */

        return -1;
    }


}

int equals(unsigned long long arg1, unsigned long long arg2){

    if (abs(arg1 - arg2) < 5000){
        return 1;
    }

    return 0;
}

int test_threshold(unsigned long long arg1, unsigned long long arg2, unsigned long long threshold){

    if (abs(arg1 - arg2) > threshold){
        return 1;
    }

    else
        return 0;
}
