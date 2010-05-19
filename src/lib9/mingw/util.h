/* await.c */
extern	int	winaddchild(int pid, HANDLE handle);

/* misc.c */
extern	char*	winpathdup(char*);
extern	int	winutftowstr(LPWSTR, char*, int);
extern	LPWSTR	winutf2wstr(char*);
extern	LPWSTR	winutf2wpath(char*);
extern	char*	winwstrtoutfm(WCHAR*);
extern	char*	winwstrtoutfe(char*, char*, WCHAR*);
extern	int	winwstrutflen(WCHAR*);
extern	int	winwstrtoutfn(char*, int, WCHAR*);
extern	int	winwstrlen(WCHAR*);


extern	void	winbsl2sl(char*);
extern	int	winisdrvspec(char*);

extern	HANDLE	wincreatefile(char*, int desiacc, int share, int creatdisp, int flagsattr);
extern	BOOL	wincreatedir(char*);
extern	HANDLE	wincreatenamedpipe(char*, int omode, int pmode, int maxinst, int outsz, int insz, int timeout);

/* cons.c */
extern	BOOL	winwritecons(HANDLE, void*, int);
extern	int	winreadcons(HANDLE, void*, int);
extern	int	winhascons(void);
extern	char*	winadjustcons(char*, int rdwr, DWORD *dap);

/* getenv.c */
extern	int	mingwinitenv(WCHAR*[]);
extern	WCHAR **winruneenv(void);
extern	void	winsortenv(char **e);

/* spawn */
extern	void	wincreaterxtxproc(int fd, int mode, HANDLE);
