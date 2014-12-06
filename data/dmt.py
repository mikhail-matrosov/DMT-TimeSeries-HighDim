# -*- coding: utf-8 -*-
"""
Created on Wed Dec 03 22:26:26 2014

@author: ZDA
"""


import math
import numpy as np
import pandas as pd
import sys
from pylab import *
import matplotlib.pyplot as plt
import argparse



def read_data_from_file(filename):
    data = []
    f = open(filename,"r")    
    for line in f:      
        date, timecol = line.split()       
        time, col = timecol.split(',')
        data.append(int(col))    
    return data
    
    
def TimeSerGen(inpt,TSlen):
    ShortTimeS = []
    LongTimeS = []
    for i in inpt:
        if (len(ShortTimeS) < TSlen):            
            ShortTimeS.append(i)            
        else:
            temp = ShortTimeS[:]
            LongTimeS.append(temp)
            del ShortTimeS[:]
            ShortTimeS.append(i) 
            
    if(len(ShortTimeS) > 0):
        temp = ShortTimeS[:]
        LongTimeS.append(temp)    
    
    return LongTimeS                      

def createParser ():
    parser = argparse.ArgumentParser()
    parser.add_argument ('-file', nargs='?',default="Dodgers.txt")
    parser.add_argument ('-tsl', nargs='?',default=1024)
    
    return parser
    

def WriteTS(TimeSeries,length):
    k = 1
    file = open("file__"+str(length)+".txt", "w")
    for i in TimeSeries:
        file.write(str(k) + " ")
        for j in i:   
            if (j == -1):
                j = 0
            file.write(str(j) + " ")
        file.write('\n')
        k = k+1
    file.close()

def main():
    TimeSeries = []
    parser = createParser()
    namespace = parser.parse_args() 
    TSlen = namespace.tsl        
    InputFile = namespace.file
    inpt = read_data_from_file(InputFile)
    print TSlen
    print InputFile
    file = open("file__total.txt", "w")       
    for i in inpt:        
        file.write(str(i) +'\t')
    file.close()
        
    #plt.plot(range(0,len(inpt)), inpt, linewidth=1, color='red')
    #plt.show()
    TimeSeries = TimeSerGen(inpt,TSlen) 
    WriteTS(TimeSeries,TSlen) 
    return 0
if __name__ == '__main__':
    main()
    
    