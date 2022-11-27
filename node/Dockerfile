FROM node:8-alpine

WORKDIR /server
COPY . /server

RUN yarn
RUN yarn build

CMD [ "node" ]
