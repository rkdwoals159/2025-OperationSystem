# 운영체제 프로젝트: 클라이언트-서버 모델을 이용한 Concurrent 파일 서버 구현

## 프로젝트 개요

- **과목**: 운영체제 (2025 가을)
- **제목**: 클라이언트-서버 모델을 이용한 concurrent 파일 서버 구현
- **환경**: Linux (MAC OS에서 개발 및 CentOS기반 AWS LINUX OS 에서 테스트)

## 프로젝트 목표

Named pipe를 통한 프로세스 간 통신을 구현하고, fork를 이용한 concurrent 파일 서버를 개발하여 운영체제의 핵심 개념들을 실습한다.

## 시스템 구조

### 아키텍처

```
[Client Process] ←→ [Named Pipe] ←→ [Server Process]
                                      ↓
                                   [Child Process]
                                      ↓
                                   [File I/O]
```

### 주요 구성 요소

1. **서버 프로세스**: Named pipe 생성 및 클라이언트 요청 처리
2. **클라이언트 프로세스**: 사용자 입력 및 파일 접근 요청
3. **Child 프로세스**: 실제 파일 I/O 작업 수행 (concurrency 구현)

## 파일 구조

```
2025-OperationSystem/
├── server.c          # 서버 프로그램
├── client.c          # 클라이언트 프로그램
├── .gitignore        # Git 무시 파일 설정
└── README.md         # 프로젝트 설명서
```

## 주요 기능

### 1. Named Pipe 통신

- 서버에서 `/tmp/server_fifo` 생성 (클라이언트 → 서버 요청용)
- 클라이언트별 고유 FIFO (`/tmp/client_{PID}_fifo`) 생성 (서버 → 클라이언트 응답용)
- 두 개의 단방향 pipe를 사용하여 양방향 통신 구현

### 2. 파일 접근 기능

- **읽기 모드 (r)**: 파일 내용을 읽어서 클라이언트에 전송
- **쓰기 모드 (w)**: 클라이언트 데이터를 파일에 저장

### 3. Concurrency 구현

- 서버가 요청을 받으면 `fork()`로 child process 생성
- Child process가 실제 파일 I/O 작업 수행
- 작업 완료 후 child process 종료
- Parent process는 `waitpid()`로 child 완료 대기

## 컴파일 및 실행 방법

### 1. 컴파일

```bash
gcc -o server server.c
gcc -o client client.c
```

### 2. 실행

**터미널 1 (서버 실행):**

```bash
./server
```

**터미널 2 (클라이언트 실행):**

```bash
./client
```

### 3. 사용법

1. 클라이언트 실행 후 파일명 입력
2. 접근 모드 선택 (r: 읽기, w: 쓰기)
3. 쓰기 모드의 경우 데이터 입력
4. 'exit' 입력으로 종료

## 프로토콜 명세

### 요청 형식

```
filename|mode|data|client_fifo_path
```

**예시:**

- 읽기: `test.txt|r||/tmp/client_12345_fifo`
- 쓰기: `test.txt|w|Hello World|/tmp/client_12345_fifo`

### 응답 형식

**읽기 응답:**

```
Read completed: X bytes
--- File Content ---
[파일 내용]
--- End of File ---
```

**쓰기 응답:**

```
Write completed: X bytes written
```


## 동작 확인 과정

### 1. 기본 기능 테스트

```bash
# 터미널 1
./server

# 터미널 2
./client
# 파일명: test.txt
# 모드: w
# 데이터: Hello World
# 결과: Write completed: 11 bytes written

# 파일명: test.txt
# 모드: r
# 결과: Read completed: 11 bytes + 파일 내용 출력
```
