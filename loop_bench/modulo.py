import numpy as np

A = np.random.randint(1000, size=1000)

x = 0

for i in range(len(A)):
    x = (x+A[i]) % 100

print ('seq res: ', x)

x = 0

for i in range(len(A) // 2):
    x = (x+A[i]) % 100

s1 = 0
s2 = 99
for i in range(len(A) // 2, len(A)):
    s1 = (s1+A[i]) % 100
    s2 = (s2+A[i]) % 100

r1 = x + s2 - 99
r2 = x + s1

print ('r1, r2: ', r1, r2)

if r1 >= 0:
    x = r1
else:
    x = r2


print ('par res: ', x)
