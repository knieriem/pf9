extern	void	winerror(char*);
typedef
enum Consctl {
	RawOn, RawOff,
} Consctl;
extern	void	winconsctl(Consctl);
extern	int	winconscolumns(int*);

extern	int	winexecpath(char[], char*, char **shell);


extern	int	winspawn(int[3], char*, char*[], int search);
extern	int	winspawne(int[3], char*, char*[], char*[], int search);
extern	int	winspawnl(int[3], char*, ...);

extern	int	winexecve(char*, char *[], char *[]);

extern	int	winexportfd(char[], int, int fd, int mode);
