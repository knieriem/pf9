/* await.c */
extern	int	winaddchild(int pid, HANDLE handle);

/* misc.c */
extern	char*	winpathdup(char*);
extern	LPWSTR	winutf2wstr(char*);
extern	LPWSTR	winutf2wpath(char*);
extern	void	winbsl2sl(char*);
extern	int	winisdrvspec(char*);

extern	HANDLE	wincreatefile(char*, int desiacc, int share, int creatdisp, int flagsattr);
extern	BOOL	wincreatedir(char*);
extern	HANDLE	wincreatenamedpipe(char*, int omode, int pmode, int maxinst, int outsz, int insz, int timeout);

/* cons.c */
extern	BOOL	winwritecons(HANDLE, void*, int);
extern	int	winreadcons(HANDLE, void*, int);
extern	int	winhascons(void);

/* getenv.c */
extern	int	mingwinitenv(Rune*[]);
extern	Rune **winruneenv(void);
extern	void	winsortenv(char **e);
