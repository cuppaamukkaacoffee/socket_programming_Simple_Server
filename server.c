
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

void error(char *msg)
{
   perror(msg);
   exit(1);
}

int main(int argc, char *argv[])
{
   int sockfd, newsockfd;
   int portno;
   socklen_t clilen;
   char buffer[1000];
   char *tmp, *tmp2;
   char reqPath[20];
   char req[5];
   char reqVer[10];
   char reqHost[24];
   char reqUsrAgent[128];
   char reqAcc[64];
   char reqAccLang[64];
   char reqAccEnc[64];
   char reqConn[64];
   char reqDate[64];
   char resDate[64];
   char resType[24];
   char *rep, *header, *res;
   time_t current;
   struct tm timestamp;
   int m, length, n;
   int fd;

   bzero(buffer, 256);

      
   /*sockaddr_in: Structure Containing an Internet Address*/
   struct sockaddr_in serv_addr, cli_addr;
   
   if (argc < 2) {
      fprintf(stderr,"ERROR, no port provided\n");
      exit(1);
   }
     
   
   sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (sockfd < 0) 
      error("ERROR opening socket");
   
   bzero((char *) &serv_addr, sizeof(serv_addr));
   portno = atoi(argv[1]); //atoi converts from String to Integer
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //for the server the IP address is always the address that the server is running on
   serv_addr.sin_port = htons(portno); //convert from host to network byte order
   
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) //Bind the socket to the server address
      error("ERROR on binding");

   listen(sockfd,5); // Listen for socket connections. Backlog queue (connections to wait) is 5

   while(1) {

      clilen = sizeof(cli_addr);
      /*accept function: 
         1) Block until a new connection is established
         2) the new socket descriptor will be used for subsequent communication with the newly connected client.
      */
      newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
      if (newsockfd < 0) 
         error("ERROR on accept");
      
      bzero(buffer,256);
      n = read(newsockfd,buffer,255); //Read is a block function. It will read at most 255 bytes

      /*
         경우에 따라 서버가 그냥 빈 buffer를 처리하는 경우가 있다.
         특히 새 브라우저를 열고 서버에 접근할 경우 요청 패킷에 추가하여 혹은 단독으로 빈 버퍼가
         서버에 보내지는 경우가 더러있다. 딱히 EOF와 관련이 없는 듯 하고
         명확한 이유를 파악하지 못했지만
         일단 기존 보일러 플레이트에 있는 에러처리(n < 0)에 추가해
         n == 0 일때도 에러처리를 해 놓았다. 에러처리가 없을 경우
         세그먼트 오류가 뜨고 서버는 다운된다.
      */
      if (n == 0) {
         printf("Note: ignoring empty packet...\n");
         continue;
      }
      if (n < 0) error("ERROR reading from socket");


      // 전달되는 HTTP Request를 파싱하여 콘솔에 나타낸다.
      printf("====================Pending Request=====================\n");

      /*
         Request의 첫 세개 필드를 추출하여 문자열 변수에 저장한다.
         buffer, tmp, tmp2 세개의 문자 포인터를 사용하여 공백을 기준으로
         메소드, uri, 버전 세가지 필드를 분리하여 각각 저장한다.
      */
      tmp = strstr(buffer, " ");
      m = snprintf(req, tmp-buffer+1, "%s", buffer);
      printf("Method: %s\n", req);

      tmp2 = strstr(tmp+1, " ");
      m = snprintf(reqPath, tmp2-tmp, "%s", tmp+1);
      printf("Request URI: %s\n", reqPath);

      tmp = strstr(tmp2, "\n");
      m = snprintf(reqVer, tmp-tmp2, "%s", tmp2+1);
      printf("Version: %s\n", reqVer);

      /*
         이외의 필드들은 키워드와 함께 제시되므로,
         strstr을 사용해서 해당 키워드의 위치를 파악하고
         나아가 키워드의 값들의 위치를 파악할 수 있다.
         해당 필드는 각각에 대응하는 문자열 변수에 저장되고 출력된다.
      */
      if ((tmp2 = strstr(buffer, "Host: ")) != NULL) {
         tmp2 = tmp2 + 6;
         tmp = strstr(tmp2, "\n");
         m = snprintf(reqHost, tmp-tmp2, "%s", tmp2);
         printf("Host: %s\n", reqHost);
      }
      if ((tmp2 = strstr(buffer, "Date: ")) != NULL) {
         tmp2 = tmp2 + 6;
         tmp = strstr(tmp2, "\n");
         m = snprintf(reqDate, tmp-tmp2, "%s", tmp2);
         printf("Date: %s\n", reqDate);
      }
      if ((tmp2 = strstr(buffer, "User-Agent: ")) != NULL) {
         tmp2 = tmp2 + 12;
         tmp = strstr(tmp2, "\n");
         m = snprintf(reqUsrAgent, tmp-tmp2, "%s", tmp2);
         printf("User-Agent: %s\n", reqUsrAgent);
      }
      if ((tmp2 = strstr(buffer, "Accept: ")) != NULL) {
         tmp2 = tmp2 + 8;
         tmp = strstr(tmp2, "\n");
         m = snprintf(reqAcc, tmp-tmp2, "%s", tmp2);
         printf("Accept: %s\n", reqAcc);
      }
      if ((tmp2 = strstr(buffer, "Accept-Language: ")) != NULL) {
         tmp2 = tmp2 + 17;
         tmp = strstr(tmp2, "\n");
         m = snprintf(reqAccLang, tmp-tmp2, "%s", tmp2);
         printf("Accept-Language: %s\n", reqAccLang);
      }
      if ((tmp2 = strstr(buffer, "Accept-Encoding: ")) != NULL) {
         tmp2 = tmp2 + 17;
         tmp = strstr(tmp2, "\n");
         m = snprintf(reqAccEnc, tmp-tmp2, "%s", tmp2);
         printf("Accept-Encoding: %s\n", reqAccEnc);
      }
      if ((tmp2 = strstr(buffer, "Connection: ")) != NULL) {
         tmp2 = tmp2 + 12;
         tmp = strstr(tmp2, "\n");
         m = snprintf(reqConn, tmp-tmp2, "%s", tmp2);
         printf("Connection: %s\n", reqConn);
      }

      printf("========================================================\n");

      /*
         Response에 들어갈 Date 필드의 값을 작성한다.
         작성될 당시의 시각 current을 통해 tm 구조체 timestamp를 생성하고
         이를 GMT 포맷에 맞게 strftime을 통해 바꿔 resDate에 저장. 
      */
      // current = time(0);
      time(&current);
      timestamp = *localtime(&current);
      strftime(resDate, sizeof resDate, "%a, %d %b %Y %H:%M:%S %Z", &timestamp);   
      
      /*
         요청한 파일 형식이 잘못될 수도 있으므로 에러처리를 한다.
         응답 코드는 500, 콘텐트 타입과 길이응 0으로 설정한다.
      */
      if (strstr(reqPath, ".") == 0) {
         printf("ERROR: invalid file path\n");
         header = malloc(128);
         length = 0;
         res = malloc(length + 128);
         if ((m = sprintf(header, "HTTP/1.1 500 Internal Server Error\n"
         "Date: %s\n"
         "Content-Type: %s\n"
         "Content-Length: %d\n"
         "Connection: close\n"
         "\n"
         , resDate, "NULL", length)) == 0)
            printf("error in sprintf");
      } 
      else {
         /*
            Response에 사용할 content-type을 구성하기 위해 앞서 추출한 uri로부터
            파일 확장자를 추출해 tmp에 저장한다.
            strtok을 쓸 경우 변수로 전달되는 문자 포인터(파일이름)가 수정되는 듯
            하므로 sprintf로 다시 복구해준다.
         */
         tmp = strtok(reqPath, ".");
         tmp = strtok(0, ".");
         m = sprintf(reqPath, "%s.%s", reqPath, tmp);
         
         /*
            확장자에 따라서 서로 다른 content-type을 작성한다. 해당 문자열은
            response에 필드로 들어가며 클라이언트 브라우저에서 이를 통해
            서로 다른 종류의 파일을 인식 할 수 있게 한다.
            .ico 확장자의 경우 과제에서 요구되지 않았으므로 따로 content-type 필드를
            구성하지 않고 무시한다.
         */
         if (strcmp(tmp, "pdf") == 0) {
            m = sprintf(resType, "application/%s", tmp);
         }
         else if ((strcmp(tmp, "jpeg") == 0) || (strcmp(tmp, "jpg") == 0) || (strcmp(tmp, "gif") == 0)) {
            m = sprintf(resType, "image/%s", tmp);
         }
         else if (strcmp(tmp, "mp3") == 0) {
            m = sprintf(resType, "audio/mpeg");
         }
         else {
            if (strcmp(tmp, "ico") == 0)
               continue;
            m = sprintf(resType, "text/%s", tmp);
         }
         
         /*
            Request에서 요청한 데이터 파일을 파일 디스크립터를 통해서 런타임에 불러온다.
            파일이 존재하지 않을 수 있으므로 역시 에러처리를 한다.
         */
         if ((fd = open(reqPath+1, O_RDONLY)) == -1){
            printf("ERROR: no such file\n");
            header = malloc(128);
            length = 0;
            res = malloc(length + 128);
            if ((m = sprintf(header, "HTTP/1.1 500 Internal Server Error\n"
            "Date: %s\n"
            "Content-Type: %s\n"
            "Content-Length: %d\n"
            "Connection: close\n"
            "\n"
            , resDate, resType, length)) == 0)
               printf("error in sprintf");
         }
         else {

            /*
               열람한 파일의 길이는 Request 헤더를 작성하고
               소켓버퍼를 통해 전송할 때 사용되므로
               미리 lseek를 사용해 추출한 다음 정수로 저장해둔다.
            */
            length = lseek(fd, 0, SEEK_END);
            lseek(fd, 0, SEEK_SET);
            
            // 파일 길이만큼의 공간(바이트 단위)을 확보해놓는다.
            rep = malloc(length);
            
            /*
               파일 디스크립터를 통해 파일의 데이터를
               앞서 확보한 공간 rep에 저장한다.
            */
            m = read(fd, rep, length);

            /*
               헤더와 Response가 들어갈 공간을 확보한다.
               헤더는 간략하게 구성할 예정이므로 128바이트를 확보하며
               Response는 (콘텐트 길이 + 헤더 길이) 만큼 확보한다.
            */
            header = malloc(128);
            res = malloc(length + 128);

            /*
               snprintf를 사용해서 헤더를 구성한다. 에러가 날 경우 표준출력에 로그한다.
            */
            if ((m = sprintf(header, "HTTP/1.1 200 OK\n"
               "Date: %s\n"
               "Content-Type: %s\n"
               "Content-Length: %d\n"
               "Connection: close\n"
               "\n"
               , resDate, resType, length)) == 0)
               printf("error in sprintf");
         }
      }

      /*
         memcpy를 사용해 우선 헤더를 response에 집어넣고,
         그 다음 콘텐트를 집어 넣는다.
      */
      memcpy(res, header, strlen(header));
      memcpy(res+strlen(header), rep, length);

      /*
         response를 소켓 디스크립터에 전달한다.
         에러시 내용을 출력한다.
      */
      if (n = write(newsockfd, res, strlen(header)+length) < 0)
         error("ERROR writing to socket");

      /*
         전송 후 파일 디스크립터와 소켓 디스트립터를 닫는다.
         동적할당한 공간들 역시 처리한다.
      */
      free(rep);
      free(header);
      free(res);
      close(fd);
      close(newsockfd);
   }

   close(sockfd);
   
   return 0; 
}
