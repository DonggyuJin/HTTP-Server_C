#include "httpd.h"

int main(int c, char** v)
{
  // httpd 헤더파일에 선언된 함수에 12000 포트를 적어준다.
  server_forever("12000");
  
  return 0;
}

void route()
{
  // httpd 헤더파일에 사용자가 정의한 함수에 definde한 값들이다.
  // ROUTE 시작!
  ROUTE_START()

  ROUTE_GET("/")
  {
    // GET 메소드에 정상적으로 성공하면 응답 코드 200을 출력한다.
    // 앞서 공부한 것처럼 HTTP 통신에서 캐리지 리턴하고 라인 피드를 확인할 수 있다.
    printf("HTTP/1.1 200 OK\r\n\r\n");
    // request_header에 들어가는 User-Agent 사용에 대한 값을 출력한다.
    printf("Hello! You are using %s", request_header("User-Agent"));
  }

  ROUTE_POST("/")
  {
    // POST 메소드에 정상적으로 성공하면 응답 코드 200을 출력한다.
    printf("HTTP/1.1 200 OK\r\n\r\n");
    // payload 크기도 출력한다.
    printf("WoW, Seems that you POSTed %d bytes. \r\n", payload_size);
    printf("Fetch the data using 'payload' variable.");
  }

  // ROUTE 종료!
  ROUTE_END()
}
