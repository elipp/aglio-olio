rangei8 = range(0, 8)
rangei2 = range(0, 2)
rangej = range(0, 6)
s = 1.0/64.0
#s = 1.0	#debug

print("static float glyph_texcoords [36][8] = {")

for j in rangej:
	if (j == 3 or j == 5):
		range = rangei2
	else: 
		range = rangei8
		
	for i in range:
		print("{ " + str(7.0*s*i) + ", " + str(10.0*s*j) +", " + str(7*s*i) +", " + str((10.0*s*j+8*s)) + ", ")	
		print(str(7.0*i*s + 6.0*s) +", " + str((10.0*s*j + 8*s)) +", " + str(7.0*s*i + 6.0*s) +", " + str(10.0*s*j) + " },\n")


print("};")