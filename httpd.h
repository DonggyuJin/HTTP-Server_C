#ifndef _HTTPD_H__
#define _HTTPD_H__

#include <string.h>
#include <stdio.h>

// 위에서 나오는 #ifndef 부터 #endif 까지는
// if not defined의 약자로 _HTTPD_H__ 를 정의하지 않았다면
// 밑에 정의한 내용들을 include 영역에 포함한다는 얘기이다.

// 서버 제어 함수
// 반환 값이 없는 함수를 만들기 때문에
// void로 설정하는 것 같음.

void server_forever(const char *PORT);


// 클라이언트 요청

char *method, // GET 메소드 or POST 메소드 정의
     *uri, // '/index.html'과 같은 uri 정의
     *qs, // 'id=guest&pw=guest'와 같은 uri에 붙는 값 정의 (querystring 약자로 추측)
     *prot; // 'HTTP/1.1'과 같은 프로토콜 정의 (protocol 약자)

char *payload; // POST를 위한 payload라고 명시되어 있음
int   payload_size; // payload를 위한 size 정의


// 앞서 배운 HTTP Request Header를 받기위한 name 매개변수 정의
char *request_header(const char* name);


// 구현해야 하는 함수, 기능 > 라우트 정의

void route();


// 정의한 라우트 함수를 위한 매개변수 정의하기
// 다음 조건문에서 수행하는 것은 URI와 uri 문자열을 비교한 값이 같고(and)
// METHOD와 method 문자열을 비교한 값이 같으면
// 정의한 라우트에 GET과 POST 메소드를, URI는 URI로 각각 처리한다.
// 그게 아니면 500 에러를 띄운다.
// cf. 참고로 500번대 에러란, Server Error를 의미한다.
#define ROUTE_START()        if (0) {
#define ROUTE(METHOD, URI)   } else if (strcmp(URI, uri)==0 && strcmp(METHOD, method)==0) {
#define ROUTE_GET(URI)       ROUTE("GET", URI)
#define ROUTE_POST(URI)      ROUTE("POST", URI)
#define ROUTE_END()          } else printf(\
                                 "HTTP/1.1 500 Not Handled\r\n\r\n" \
                                 "The server has no handler to the request.\r\n" \
                             );

#endif
