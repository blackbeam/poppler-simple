# -------------------------------------------------------------------------
# Good read about using Node in Docker
# https://github.com/nodejs/docker-node/blob/master/docs/BestPractices.md
#
# Build image like this:
# docker build -t name_of_image . --build-arg NPM_TOKEN=${NPM_TOKEN}
# -------------------------------------------------------------------------

# Base image
# As of now (Nov 2018), node 8 is as high as you can go.
FROM node:8-alpine

# File Author / Maintainer
MAINTAINER Your name

# Install Poppler and other dependencies
# Alpine image is small, but lacks some essentials
# - Add alpine-sdk to get build tools, python, etc
# - Add pkgconfig, poppler-simple needs this to build
# - Add poppler-dev, the standard poppler package won't do
# - Add bash if you want to run wait-for-it.sh or just interact with the shell
RUN apk --update add alpine-sdk pkgconfig poppler-dev bash

# Remove apk cache to slim down the image
RUN rm -rf /var/cache/apk/*

# Need to wait for other Docker containers before spinning up this one?
# wait-for-it will help you with this
# https://github.com/vishnubob/wait-for-it
COPY wait-for-it.sh /home/node/wait-for-it.sh
RUN ["chmod", "+x", "/home/node/wait-for-it.sh"]

#Copy .npmrc and define build time env-variable for use in .npmrc
#This is needed in order for npm to install private npm packages
#N.B. "docker build" needs to be passed your local env-variable NPM_TOKEN
# via the argument "--build-arg NPM_TOKEN"
COPY .npmrc /tmp/.npmrc
ARG NPM_TOKEN

# Define working directory, copy code and package.json
WORKDIR /home/node/app
COPY ./src /home/node/app/src
COPY package.json /home/node/app/package.json
COPY package-lock.json /home/node/app/package-lock.json

# Tell npm to put global packages in homedir instead of root
ENV NPM_CONFIG_PREFIX=/home/node/.npm-global

# Install nodemon
RUN npm install -g nodemon

# The Node Docker image has a non privileged user "node" for running node processes. Let's use it
USER node

# Run app using nodemon
# In production this would probably be PM2, but I find nodemon easier to work with in development
#CMD ["node", "./src/start.js"]
