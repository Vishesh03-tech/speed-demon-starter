FROM alpine:3.18 AS builder
RUN apk add --no-cache g++
WORKDIR /build
COPY main.cpp .
RUN g++ -O3 -march=native main.cpp -o main

FROM alpine:3.18
RUN apk add --no-cache libstdc++
WORKDIR /app
COPY --from=builder /build/main .

ENTRYPOINT ["./main"]