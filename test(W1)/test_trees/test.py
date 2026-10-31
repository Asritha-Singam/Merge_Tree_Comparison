import matplotlib.pyplot as plt
import subprocess
x_list = []
y_list = []
for i in range(1,251):
    print(i)
    command = "java -cp ../../CustomisedWasserstein CustomisedWasserstein.java tv_0.jt tv_" + str(i) + ".jt ./"  # the shell command
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None, shell=True)
    #Launch the shell command:
    output = float(process.communicate()[0].decode('utf-8').strip())
    y_list.append(output)
#print(y_list)
for i in range(1,251):
    x_list.append(i)

plt.plot(x_list,y_list)
plt.xlabel("Tree")
plt.ylabel("W1 Distance")
plt.title("Testing Wasserstein(W1)")
plt.ylim(0,1)
plt.show()
