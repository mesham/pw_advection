f1 = open("input_A.txt", "w")
f2 = open("input_B.txt", "w")
# 240 as each stream reads 3 times per cell, therefore 80 cells here
f1.write("10485 0 0 0\n")
for line in range(10485):
  f1.write(str(float(line))+" "+str(float(line+1))+" "+str(float(line+2))+" "+str(float(line+3))+"\n")
  f1.write(str(float(line))+" "+str(float(line+1))+" "+str(float(line+2))+" "+str(float(line+3))+"\n")
  f2.write(str(float(line+1000))+" "+str(float(line+1001))+" "+str(float(line+1002))+" "+str(float(line+1003))+"\n")
  f2.write(str(float(line+1000))+" "+str(float(line+1001))+" "+str(float(line+1002))+" "+str(float(line+1003))+"\n")
f1.close()
f2.close()
