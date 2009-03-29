enum {
	Fdtypenone,
	Fdtypesock,
	Fdtypepipecl,
	Fdtypepipesrv,
	Fdtypefile,
	Fdtypestd,
	Fdtypecons,
	Fdtypedir,
	Fdtypedevnull,
};

typedef
struct Fd {
	char *name;

	int	type;

	int	users;

	SOCKET	s;

	HANDLE	h;
	struct {
		HANDLE	r, w;
	} ev;

	void	*dir;

	struct {
		QLock	r, w, rw;
	} lk;

} Fd;

extern	Fd	**fdtab;
extern	int	fdtabsz;

extern	void	fdtabinit(void);

extern	int	fdtalloc(Fd*dupf);
extern	int	fdtdup(int oldfd, int newfd);
extern	int	fdtclose(int i);

extern	void	fdtprint(void);

extern	Fd*	fdtget(int fd);

extern	int	fdtdebug;
