# 使用多阶段构建
FROM ubuntu:22.04 as builder

# 安装构建依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libomp-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# 复制源代码
COPY . /src
WORKDIR /src

# 构建项目
RUN mkdir build && cd build \
    && cmake .. \
    && make -j$(nproc)

# 运行阶段
FROM ubuntu:22.04

# 安装运行时依赖
RUN apt-get update && apt-get install -y \
    libomp5 \
    nlohmann-json3 \
    && rm -rf /var/lib/apt/lists/*

# 复制构建产物
COPY --from=builder /src/build/server /app/server
COPY --from=builder /src/build/libgengine.* /app/lib/

# 设置工作目录
WORKDIR /app

# 暴露端口
EXPOSE 8081

# 启动服务
CMD ["./server"] 