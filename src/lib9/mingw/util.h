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

extern	void	winreplacews(WCHAR*, int rev);

extern	void	winbsl2sl(char*);
extern	int	winisdrvspec(char*);

extern	HANDLE	wincreatefile(char*, int desiacc, int share, int creatdisp, int flagsattr);
extern	BOOL	wincreatedir(char*);
extern	int	wincreatenamedpipe(HANDLE*, char* , int mode, int n);
extern	int	winconnectpipe(HANDLE, int needclient);
extern	int	wincreatevent(HANDLE*, char*, int manrst, int ini);

/* pipe.c */
extern	char* wincreatepipename(char[], int);

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
