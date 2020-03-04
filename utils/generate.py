import re
import sys
#[2020-03-03 10:50:45.513741 CET] [151530] [debug] <read():315> host: 9, path: /fluidda-XFIEL.00000001.00000119.mpio.bin, chunks: 1, size: 524288, offset: 295200
#(read).+(host: )(\d+).+(path: )(.+),.+(chunks: )(\d+).+(size: )(\d+).+(offset: )(\d+)
# Original host # path # chunkid # size # offset 
destination = sys.argv[1]
file = sys.argv[2]
pattern = re.compile(r".+(read).+(host: )(\d+).+(path: )(.+),.+(chunks: )(\d+).+(size: )(\d+).+(offset: )(\d+)")

with open(file) as f:
	for line in f:
		result = pattern.match(line)
		if result:
			#split = re.split(":\,",line)
			print (result[1], result[3], result[5], result[7], result[9], result[11], destination)