import sys
with open(sys.argv[1], 'r') as f:
	lines = f.readlines()
	for line in lines:
		i = 0
		while line[i] == " ":
			i += 1;
		newLine = ' '*i + line;
		print(newLine, end='')