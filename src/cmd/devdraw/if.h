extern void	_xmoveto(Point);

struct Cursor;
extern void	_xsetcursor(struct Cursor*);

struct Mouse;
extern void	_xbouncemouse(struct Mouse*);

extern int		_xsetlabel(char*);
extern Memimage*	_xattach(char*, char*);
extern char*		_xgetsnarf(void);
extern void		_xputsnarf(char *data);
extern void		_xtopwindow(void);
extern void		_xresizewindow(Rectangle);
