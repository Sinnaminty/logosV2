#!/bin/bash

docker build -t logos.
docker run -d --name logos-container logos
