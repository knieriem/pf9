int _drawmsgread(void*, int);
int _drawmsgwrite(void*, int);
void _initdisplaymemimage(Memimage*);
int _latin1(Rune*, int);

struct Wsysmsg;
void	runmsg(struct Wsysmsg*);
