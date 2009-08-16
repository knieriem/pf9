#include <u.h>
#include <mingwutil.h>
#include <libc.h>

static int
rawx(int fd, int echoing)
{
	int was;

	if(echoing == -1)
		return -1;

	was = RawOff;
	if (echoing==0)
		winconsctl(RawOn);
	else
		winconsctl(echoing);
	return was;
}

char*
readcons(char *prompt, char *def, int secret)
{
	int fd, n, raw;
	char line[10];
	char *s, *t;
	int l;

	if((fd = open("/dev/tty", OREAD)) < 0)
		return nil;

	raw = -1;
	if(secret){
		raw = rawx(fd, 0);
		if(raw == -1)
			return nil;
	}

	if(def)
		fprint(2, "%s[%s]: ", prompt, def);
	else
		fprint(2, "%s: ", prompt);

	s = strdup("");
	if(s == nil)
		return nil;

	for(;;){
		n = read(fd, line, 1);
		if(n < 0){
		Error:
			if(secret){
				rawx(fd, raw);
				write(fd, "\n", 1);
			}
			close(fd);
			free(s);
			return nil;
		}
		if(n > 0 && line[0] == 0x7F)
			goto Error;
		if(n == 0 || line[0] == 0x04 || line[0] == '\n' || line[0] == '\r'){
			if(secret){
				rawx(fd, raw);
				write(fd, "\n", 1);
			}
			close(fd);
			if(*s == 0 && def){
				free(s);
				s = strdup(def);
			}
			return s;
		}
		if(line[0] == '\b'){
			if(strlen(s) > 0)
				s[strlen(s)-1] = 0;
		}else if(line[0] == 0x15){	/* ^U: line kill */
			if(def != nil)
				fprint(2, "\n%s[%s]: ", prompt, def);
			else
				fprint(2, "\n%s: ", prompt);
			s[0] = 0;
		}else{
			l = strlen(s);
			t = malloc(l+2);
			if(t)
				memmove(t, s, l);
			memset(s, 'X', l);
			free(s);
			if(t == nil)
				return nil;
			t[l] = line[0];
			t[l+1] = 0;
			s = t;
		}
	}
}
