#
# See blog post at
# https://lemire.me/blog/2019/06/27/bounding-the-cost-of-the-intersection-between-a-small-array-and-a-large-array/
#
maxval = 4000000000000
large = 4000000
small = 1024
import random
from math import *

def naivemodel(small,large):
    return small * log2 (large)


print("naive model :", naivemodel(small,large))
print("model over small:", naivemodel(small,large) / small)

def model(small,large):
    return (large + 0.5) * log2((large+0.5)/(large -small+0.5)) + small * log2((large - small +0.5)/small) -0.5*log2(2*pi*small)
print("model :", model(small,large))

print("model over small:", model(small,large) / small)


def prune(a):
    for i in range(len(a)-1,0,-1):
      if a[i] == a[i-1]:
        del a[i]
print("building large array")
largearray = [random.randint(0,maxval) for i in range(large)]
largearray.sort()
prune(largearray)
print("got ",len(largearray))
while(len(largearray) < large):
    shortby = large - len(largearray)
    for i in range(shortby):
        newval = random.randint(0,maxval)
        largearray.append(newval)
    largearray.sort()
    prune(largearray)

print("building small array")
smallarray = [largearray[random.randint(0,large)] for i in range(small)]
smallarray.sort()
prune(smallarray)
while(len(smallarray) < small):
    newval = largearray[random.randint(0,large)]
    try:
        smallarray.index(newval)
    except:
        smallarray.append(newval)
smallarray.sort()

dataaccesses = []



def simulate_binary_search(largearray, key,datacesses) :
    low = 0
    decision = 0
    high = len(largearray) - 1
    span = len(largearray)
    while(span > 1) :
        span = span // 2
        middleindex = low + span
        decision += 1
        middlevalue = largearray[middleindex]
        datacesses.append(middleindex)
        if middlevalue < key:
            low = middleindex + 1
        else:
            high = middleindex
    return decision

print("starting simulation")
decision = 0
for x in smallarray:
    decision += simulate_binary_search(largearray,x,dataaccesses)

print("number of decisions per key ", decision / small)
print("number of total accesses", len(dataaccesses))
print("number of total accesses over small", len(dataaccesses)/small)

dataaccesses.sort()
prune(dataaccesses)
print("number of distinct accesses", len(dataaccesses))
print("number of distinct accesses over small", len(dataaccesses)/small)

for i in range(len(dataaccesses)):
    dataaccesses[i] = dataaccesses[i] // 16
prune(dataaccesses)
print("number of line accesses", len(dataaccesses))
print("number of line accesses over small", len(dataaccesses)/small)
