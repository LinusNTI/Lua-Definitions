#DEFINE P2V(x1,y1, w,h) {x=x1/w, \
	y=y1/h}

#DEFINE PD2PF(p) (p*100)

print(type(P2V(960,540, 1920,1080)))
print(type(PD2PF(0.97)))