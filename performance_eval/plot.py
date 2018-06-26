import matplotlib.pyplot as plt
import matplotlib

f = open('speedups.txt', 'r')

first = True

testcases = {}

for line in f:
    # skip the first line
    if first:
        first = False
        continue

    data = line.split()

    loop_name = data[0].split('-')[0]

    if loop_name not in testcases:
        testcases[loop_name] = [data]
    else:
        testcases[loop_name].append(data)

x = range(7)

for loop_name, results in testcases.items():
    print ('plotting ' + loop_name + ' ...')
    fig, ax = plt.subplots()
    for res in results:
        ax.plot(x, [float(res[i]) for i in range(1,8)], label=res[0])
    plt.xticks(range(7), ('1', '2', '4', '8', '16', '32', '64'))
    ax.set_yscale('log', basey=2)
    ax.legend()
    plt.gca().yaxis.grid(True, linestyle='--')
    ax.yaxis.set_major_formatter(matplotlib.ticker.StrMethodFormatter('{x:.0f}'))
    ax.set_ylabel('Number of Threads')
    ax.set_xlabel('Speedup')
    plt.title(loop_name)
    plt.savefig(loop_name + '.pdf')
    plt.close(fig)



