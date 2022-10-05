#include "httpd.h"
// 앞서 만들어준 httpd.h 헤더파일을 포함한다.

#include <stdio.h> // 표준 입출력을 위한 헤더파일
#include <string.h> // 문자열 함수를 위한 헤더파일
#include <stdlib.h> // atoi, abs와 같은 함수를 포함하는 헤더파일
#include <unistd.h> // unix에서 사용하는 C 컴파일러 헤더파일, 여기서부터 C POSIX library.h
#include <sys/types.h> // 다양한 데이터 유형을 사용하는 헤더파일
#include <sys/stat.h> // 파일 정보(통계분석)등을 하는 헤더파일
#include <sys/socket.h> // 메인 소켓 헤더파일
#include <arpa/inet.h> // 인터넷 operation들을 정의한 헤더파일
#include <netdb.h> // 네트워크 데이터베이스 operation들에 대한 헤더파일
#include <fcntl.h> // 파일 제어 옵션에 대한 헤더파일
#include <signal.h> // 신호에 관련된 헤더파일

#define CONNMAX 1000

// 정적 자료형으로 정의
// listenfd가 listen file descriptor를 말하는 것 같은데,
// fd는 네트워크 소켓과 같은 파일이나 I/O 리소스에 접근하는데 사용되는 개념이다.
static int listenfd, clients[CONNMAX];
static void error(char *);
static void startServer(const char *);
static void respond(int);

// _t는 type의 약자로 typedef로 정의된 것을 의미한다.
typedef struct { char *name, *value; } header_t;

// 아마도 reqhdr이 request header 약자로 보이고, 배열 크기와 함께 선언한 뒤
// NULL로 초기화해주는 듯하다. (나름 혼자 추리중,,)
static header_t reqhdr[17] = { {"\0", "\0"} };
static int clientfd; // 이번에는 client의 file descriptor를 의미하는 듯하다.

static char *buf;


void server_forever(const char *PORT) // 함수를 하나 정의해주고 PORT를 매개변수로 받는다
{
  struct sockaddr_in clientaddr;
  // socklen_t는 정의한 적이 없는데..? 라고 생각해서 검색해보았다.
  // socklen_t란 소켓 관련 매개 변수에 사용되고, 길이 및 크기 값에 대한 정의를 한다.
  socklen_t addrlen; // socket.h 헤더파일에 정의되어 있음
  char c;

  int slot=0;

  // 여기서 나오는 \033[92m 이나 \033[0m은 색상을 의미하는 걸로 검색하면 나오는데,
  // 92m은 python에서 OKGREEN, 0m은 ENDC라고 한다. (이후에 해보면 알겠지,,)
  printf(
      "Server started %shttp://127.0.0.1:%s%s\n",
      "\033[92m", PORT, "\033[0m"
      );

  // 모든 요소를 -1로 세팅하면 연결된 클라이언트가 없다는 것을 나타낸다.
  // CONNMAX가 1000으로 선언되어 있고 0부터 999까지 -1로 세팅되고,
  // 정의한 startServer에 포트를 매개변수로 넣어준다.
  int i;
  for (i=0; i<CONNMAX; i++)
    clients[i] = -1;
  startServer(PORT);

  // SIGCHLD란 시그차일드, 시그널 차일드라고 읽는 신호로
  // 자식 프로세스가 종료될 때 부모 프로세스에 통보되는 POSIX 신호이다.
  // 해당 신호를 통해 부모 프로세스는 자식 프로세스의 종료 코드를 얻을 수 있다.
  // 여기서 나오는 POSIX 신호는 SIGINT, SIGKILL, SIGCHLD 등이 있다.
  // 좀비 쓰레드를 회피하기 위해서 SIGCHLD를 무시한다고 설명되어 있다.
  signal(SIGCHLD, SIG_IGN); // SIGCHLD를 무시한다. signal(시그널, SIG_IGN)

  // 연결을 수립하는 과정이다.
  while (1) // True인 동안의 While 반복문
  {
    addrlen = sizeof(clientaddr); // 클라이언트 주소 길이를 받는다.
    // 0으로 초기화된 슬롯 clients[0]에 accpet 함수를 사용한다.
    // accept 함수는 연결지향 소켓 타입에 사용되는데, 새로운 연결된 소켓을 만들고
    // 소켓을 가르키는 파일 지정자를 할당하고 리턴한다고 설명되어있다.
    // listenfd가 socket()으로 만들어지는 end-point(listen socket)을 위한 파일지정자이다.
    // sockaddr로 선언된 구조체 포인터에는 clientaddr의 주소값이 들어가는데,
    // 해당 인자는 sockaddr 구조체에 대한 포인터로 연결에 성공하면 해당 구조체를 채워서
    // 되돌려주고 이 구조체 정보를 이용해 클라이언트 인터넷 정보를 알아낼 수 있다.
    // addrlen 인자는 위에서 선언한 클라이언트 주소 길이를 받아 addr 크기로 넣게된다.
    clients[slot] = accept (listenfd, (struct sockaddr *) &clientaddr, &addrlen);

    if (clients[slot] < 0) // 연결된게 없으면 에러를 발생시킨다.
    {
      perror("accept() error"); // perror 함수는 오류 메세지를 stderr로 출력한다.
      // 참고로 stderr로 출력되는 메시지는 버퍼링 없이, 즉시 출력된다.
    }
    else
    {
      if (fork() == 0)
      {
        respond(slot); // respond(int)로 선언된 함수에 slot 값을 넣는다.
        exit(0); // exit() 함수는 프로세스 종료이고, 0은 정상종료를 의미한다.
      }
    }

    // clients[slot]이 -1이 아니라면 slot에 1씩 더한 값을
    // 1000으로 나눈 나머지를 slot 값에 넣는다.
    // 이게 반복되면 나머지는 1~1000 값으로 계속해서 돌게 된다.
    // 1000까지의 범위에 대해서 의미한 듯하다.
    while (clients[slot] != -1) slot = (slot+1) % CONNMAX;
  }
}


// 서버를 시작한다.
void startServer(const char *port) // 위에서 사용한 함수를 이제 정의한다
{
  // 주소의 정보에 대한 매개변수를 addrinfo 구조체에 정의하는 부분같다.
  struct addrinfo hints, *res, *p;

  // 호스트에 대한 getaddrinfo 함수에 담아주는 내용인 듯하다.
  // memset 함수는 메모리의 내용을 원하는 크기만큼 특정 값으로 세팅, 초기화한다.
  // 즉, 여기서는 hints 메모리의 주솟값, 세팅하고자하는 값, 길이를 받는다.
  memset (&hints, 0, sizeof(hints));
  // learn.microsoft.com에서 서버에 대한 소켓 만들기를 참고한 내용에 따르면,
  // AF_INET은 IPv4 주소 패밀리를 지정하는데 사용한다.
  // SOCK_STREAM은 스트림 소켓을 지정하는데 사용한다.
  // AI_PASSIVE 플래그는 호출자가 바인딩 함수 호출에 반환된 소켓 주소 구조 사용을 나타낸다.
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  // getaddrinfo 함수는 ANSI 호스트 이름에서 주소로 프로토콜 독립적 변환을 제공한다.
  // NULL로 노드를, port로 서비스를, 주소 정보를 넘길 구조체와 가져올 네트워크 주소 정보를.
  if(getaddrinfo(NULL, port, &hints, &res) != 0)
  {
    perror("getaddrinfo() error");
    exit(1); // 1은 에러메시지 종료를 의미한다.
  }
  
  // 소켓과 바인드하는 과정이다.
  // 인자로 받는 값 p는 response의 값이고, NULL이 아닐때까지,
  // 구조체 포인터(*res)가 ai_next 주소에 데이터를 할당한다.
  // 화살표 연산은 구조체 포인터 안의 변수에 접근하게 만든다.
  for (p=res; p!=NULL; p=p->ai_next)
  {
    int option = 1;
    
    // socket 함수로 소켓을 만들어준다.
    // ai_family는 AF_INET이라는 IPv4 영역을 말하고 해당 영역의 통신으로 지정한다.
    // ai_socktype은 SOCK_STREAM으로 TCP 타입의 프로토콜 사용을 설정한다.
    // 0은 protocol 자리로 0을 써도되고, TCP일때는 IPPROTO_TCP를 사용해도 된다.
    
    listenfd = socket (p->ai_family, p->ai_socktype, 0);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    // setsockopt는 소켓의 옵션 값을 변경하기 위해 사용한다.
    // listenfd로 소켓지정번호를 사용하고, SOL_SOCKET 레벨로 소켓정보를 획득, 변경한다.
    // 참고로 level 자리에는 SOL_SOCKET 또는 IPPROTO_TCP 중 하나를 사용한다고 한다.
    
    // SO_REUSEADDR은 설정을 위한 소켓옵션의 번호이고,
    // &option은 설정 값을 저장하기 위한 버퍼의 포인터로 사용한다.
    // 마지막은 위의 optval(&option) 버퍼의 크기를 명시한다.
    // 참고로 SO_REUSEADDR은 이미 사용된 주소를 재사용(bind)하게 한다.
    
    // listenfd가 -1 값이면 다음 문장을 실행한다.
    // 그 다음, bind 함수를 작성하고 해당 반환값이 0이면 탈출한다.
    // 여기서 등장하는 bind 함수는 소켓에 주소를 할당해주는 함수이다.
    // listenfd 자리는 sockfd 자리로 소켓의 식별자 내지 소켓 디스크립터를 의미한다.
    // 현재 AF_INET 통신을 하기 때문에 struct sockaddr이 아닌 struct sockaddr_in을 사용한다.
    // sockaddr_in 멤버에는 소켓 주소체계, 포트, IP, zero[8]이 있는데,
    // 여기서는 구조체 포인터인 p를 ai_addr을 가리키게 만들어 주소를 전달한다.
    // 참고로 ai_addr은 struct sockaddr 구조체에 포함되어 있는 멤버이다.
    // 마지막 자리는 ai_addr 구조체의 크기를 가리킨다.
    if (listenfd == -1) continue;
    if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
  }

  // p가 모든 값을 잘 수행했다면, socket과 bind 함수가 실행됐을 것이다.
  // 그게 아니라면 오류를 출력한다.
  if (p == NULL)
  {
    perror ("socket() or bind()");
    exit(1);
  }

  // getaddrinfo 함수가 동적으로 할당해주는 모든 구조체 메모리의
  // 해당 함수를 호출한 곳에서 해당 구조체의 메모리를 해제한다.
  freeaddrinfo(res);

  // 들어오는, 요청하는 연걸에 대한 listen 상태로 설정한다.
  // listenfd로 소켓 디스크립터를 설정하고,
  // 연결 요청을 대기시킬 공간을 1000000으로 설정한다.
  // 연결 성공은 0을, 실패는 -1을 반환한다.
  if (listen (listenfd, 1000000) != 0)
  {
    perror("listen() error");
    exit(1);
  }
}


// Request Header를 가져오는 부분이다.
char *request_header(const char* name)
{
  // 여기서 나오는 header_t는 최상단에 정의해 구조체 배열로
  // 매개변수 name과 value를 받고 있는 것을 확인한다.
  // 구조체 포인터 h가 name을 가리키는 동안에
  // 가리킨 name 값과 name이 일치하면 h는 value를 가리킨다.
  // reqhdr = {{}}; 과 같은 모양이였으므로
  // name의 값이 같을때는 더이상 찾을 필요가 없어서 value로 채우고
  // h++을 해주어 다음 name을 비교해준다는 의미인 듯하다.
  header_t *h = reqhdr;
  while(h->name)
  {
    if (strcmp(h->name, name) == 0) return h->value;
    h++;
  }
  return NULL;
}


// 이제 클라이언트 연결을 해줄 차례이다.
void respond(int n) // 최상위에서 respond 함수를 정의해주고 사용한적이 있다.
{
  int rcvd, fd, bytes_read;
  char *ptr;

  // SIZE_MAX 65535에 대한 메모리를 동적 할당한다.
  // recv 함수는 소켓으로부터 데이터를 수신하는 함수이다.
  // 위에서 slot으로 받은 int 값을 clients 소켓 디스크립터로 받는다.
  // 수신할 버퍼 포인터 buf를 받고, 버퍼의 길이인 65535를 입력한다.
  // 마지막으로 옵션을 주지 않을때는 0을 선언하고,
  // MSG_OOB, _PEEK, _WAITALL와 같은 옵션을 줄 수도 있다.
  buf = (char *) malloc(65535);
  rcvd = recv(clients[n], buf, 65535, 0);

  if (rcvd < 0) // 수신에 실패하면
    fprintf(stderr, ("recv() error\n")); // 파일 스트림에 값을 전달하여 출력한다.
  else if (rcvd == 0) // 수신 소켓이 닫혀있다면
    fprintf(stderr, "Client disconnected upexpectedly.\n"); // 예기치않게 종료를 출력한다.
  else // 수신하면
  {
    buf[rcvd] = '\0'; // buf에 rcvd로 받은 리턴값만큼의 배열을 NULL로 채운다.
    // 이때, recv() 함수의 리턴값은 최소 1, 최대 실제 송신한 바이트 수 len을 반환한다.

    // strtok은 string + tokenize로 문자열을 자르는 함수이다.
    // buf을 받아서 \t\r\n에 대한 구분자를 이용하여 자른뒤 메소드 값으로 저장한다.
    // 여기서 나오는 제어문자 \t는 탭이고, \r은 현재 줄에서 첫 부분으로 이동하는 것이다.
    // 그럼 왜? 제어문자 \t와 \r이 오는가? 했을 때 이유는 다음과 같다.
    // GET / HTTP/1.1 과 같은 HTTP 요청이 있을 때,
    // 해당 라인 끝에는 캐리지 리턴과 개행(라인 피드)가 꼭 들어가야한다고 한다.
    method = strtok(buf, " \t\r\n");
    uri    = strtok(NULL, " \t");
    prot   = strtok(NULL, " \t\r\n");

    // 해당 부분은 stderr에 Green 색깔과 끝을 명시하고,
    // method와 uri에 대한 에러 로그를 기록하는 부분인 듯하다.
    fprintf(stderr, "\x1b[32m + [%s] %s\x1b[0m\n", method, uri);

    if (qs = strchr(uri, '?')) // querystring 값과 uri에 물음표를 찾은 값이 일치하면
    {
      *qs++ = '\0'; // URI를 split 한다.
    }
    else // 그게 아니라면 빈 문자열을 작성한다.
    {
      qs = uri - 1;
    }

    // 여기서도 header_t를 다시 명시한다.
    header_t *h = reqhdr;
    char *t, *t2;

    // request_header를 통해 받은 헤더에 대해
    // h->name을 가리키는 값은 k에
    // h->value를 가리키는 값은 v에 저장한다.
    while (h < reqhdr + 16)
    {
      char *k, *v, *t;
      
      k = strtok(NULL, "\r\n: \t"); if (!k) break;
      v = strtok(NULL, "\r\n");     while (*v && *v == ' ') v++;
      
      h->name  = k;
      h->value = v;
      h++;

      fprintf(stderr, "[H] %s: %s\n", k, v);

      t = v + 1 + strlen(v);
      if (t[1] == '\r' && t[2] == '\n') break;
    }
    t++; // 포인터 t는 사용자 페이로드의 시작지점이 되었다는 것을 의미한다.
    t2 = request_header("Content-Length"); // 시작점이 있다면 연관된 헤더가 된다.
    
    // payload 크기는 Content-Length라는 헤더가 true이면,
    // ascii to long, ascii를 long 타입으로 변환하고
    // false면 아까 recv 함수로 받은 rcvd 값에 시작점 - buf만큼을 크기로 한다.
    payload = t;
    payload_size = t2 ? atol(t2) : (rcvd - (t-buf));

    // clientfd를 표준 출력 stdout에 바인딩해서 쓰기 쉽게 만들어준다.
    clientfd = clients[n];
    // dup2 함수는 파일 서술자를 복제하는 함수이다.
    dup2(clientfd, STDOUT_FILENO); // 새 서술자의 값을 STDOUT_FILENO로 지정한다.
    close(clientfd); // socket 함수로 생성된 소켓을 종료한다.

    // 참고로 stdin <-> STDIN_FILENO 표준 입력
    // stdout <-> STDOUT_FILENO 표준 출력
    // stderr <-> STDERR_FILENO 표준 에러를 의미한다.
    
    // 라우터를 호출한다.
    // 여기서 호출한 route() 함수는 httpd.h 헤더 파일에 정의한 함수이다.
    route();

    // 마지막으로 남은 것들을 정리한다.
    fflush(stdout); // 파일 스트림 stdout을 비운다. (버퍼에 있는 데이터들 삭제)
    
    // 성공하면 0, 실패하면 -1을 반환하는 소켓 종료 함수 shutdown으로
    // 여기서는 send buffer를 차단하는 SHUT_WR을 작성한다.
    // SHUT_WR을 호출하면 더이상 해당 소켓에게 송신할 수 없게 된다.
    shutdown(STDOUT_FILENO, SHUT_WR);
    close(STDOUT_FILENO); // 마지막으로 소켓을 종료하고 이후부터는 통신을 주고받을 수 없다.
  }

  // SOCKET을 최종적으로 closing 한다.
  shutdown(clientfd, SHUT_RDWR); // 이후 모든 송수신을 할 수 없게 recv, send 버퍼를 차단한다.
  close(clientfd);
  clients[n] = -1;
}
