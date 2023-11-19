FROM balenalib/raspberrypi400-64-debian

WORKDIR /PCA9685

COPY . .

RUN apt update
RUN apt install cmake build-essential

RUN mkdir build

WORKDIR /PCA9685/build

RUN cmake ..

RUN make

RUN make install

# ola deps
RUN apt install ola libola-dev

# audio deps
# RUN apt install libasound2 libasound2-dev libfftw3-3 libfftw3-dev

# demo deps
# RUN apt install libncurses5-dev

RUN make examples

CMD ["sleep", "7200"]