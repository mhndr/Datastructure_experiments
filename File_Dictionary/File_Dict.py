import os

index = {}
db_file = "./test.db"

def get(key):
	if key not in index.keys():
		return "NOT FOUND"
	db_fd = open(db_file,"r")
	db_fd.seek(index[key])
	value = db_fd.readline()
	db_fd.close()
	return value

def load_index():
	db_fd = open(db_file,"r")
	if not db_fd:
		print("couldn't open db file")
	print("opened db file")
	while line := db_fd.readline():
		if line.count(',') == 0:
			continue
		key,val = line.split(',')
		index[key]= db_fd.tell()-len(val)
	db_fd.close()
	
def put(key,value):
	if key in index.keys():
		old_value = get(key)
		db_fd = open(db_file,'r+')
		db_fd.seek(index[key]-len(key)-1)
		db_fd.write("-"*(len(key)+len(old_value))+"\n") #strike out record
		db_fd.close()

	db_fd = open(db_file,'a')
	db_fd.write(key+","+value+"\n")
	db_fd.close()
	index.clear()
	load_index()
	return	
	# approach 2
	#write to changelog file
	#when update thread runs it will merge the files
	
def clean_file():
	pass

def patch(patch_file_name):
	patch_fd = open(patch_file_name,"r")
	if not patch_file_name:
		print("couldn't open patch file")
		return
	patch = patch_fd.readlines()
	for line in patch:
		if line.count(',') == 0:
			continue
		key,val = line.split(',')
		put(key,val)

load_index()
while True:
	_input = input("").strip()
	nTokens = _input.count(" ")+1
	tokens = _input.split(" ")
	if nTokens == 0:
		continue
	elif nTokens == 1:
		cmd = tokens[0]
		if cmd == "EXIT":
			exit(0)
		if cmd == "CLEAN":
			clean_file()
			continue
	elif nTokens == 2:
		cmd,data = tokens
		if cmd == "GET":
			print(get(data))
			continue
		elif cmd == "PATCH":
			patch(tokens[1])
			continue
	elif nTokens >= 3:
		cmd = tokens[0]
		if cmd == "PUT" and tokens[1] and tokens[2]:
			put(tokens[1]," " .join(tokens[2:]))
			print("DONE")
			continue
	
	if len(_input)!=0:
		print("unknown command")
		continue
"""
opened db file
GET apple
malum

GET ball
a round object

GET cat 
a pet animal

GET violin
a musical instrument

GET book
a reading object

GET pen
mightier than a sword

GET laptop
NOT FOUND
PUT laptop
unknown command
PUT laptop computer
opened db file
DONE
GET laptop
computer

EXIT
_______________________________________
% cat test.db
-------------
-------------
-------------
----------------
--------------------
apple,malum
ball,a round object

violin,a musical instrument

pen,mightier than a sword
-------------
book,a reading object
cat,a pet animal
laptop,computer



"""
