FROM emscripten/emsdk:latest
WORKDIR /app
COPY . .

RUN mkdir -p embuild && cd embuild && \
    emcmake cmake .. && \
    cmake --build .

RUN npm install -g serve

EXPOSE 3333
CMD  ["npx", "-s", "serve", "-s", "embuild/build", "-p", "3333"]