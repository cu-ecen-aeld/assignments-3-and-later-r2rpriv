#include <syslog.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>


int main(int argc, char **argv){
   int writeFd = -1;
   // Open syslog with identifier, setting option to NODELAY and USER facility.
   openlog("Assignment@:writer",LOG_NDELAY,LOG_USER);

   if(argc != 3){
   	syslog(LOG_ERR,"insufficient arguments, command is: %s %s %s",argv[0],"filename","string");
	return 1;
   }

   writeFd = open(argv[1], O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP);
   if(writeFd < 0){
	syslog(LOG_ERR,"Unable to open file %s -- error %s",argv[1],strerror(errno));
	return 1;
   }

   if(write(writeFd,argv[2],strlen(argv[2])) < 0) {
	   syslog(LOG_ERR,"Error writing file %s -- error %s",argv[1],strerror(errno));
	   return 1;
   }
   syslog(LOG_DEBUG,"Writing %s to %s",argv[2],argv[1]);
   closelog();
   return 0;
}
