# Nginx Debug module

## Background

Nginx is a widely used web server. However, when a web application become 
complicated, it is a nightmare to maintain the Nginx configuration. For example:
1. It is hard find the location a certain request matched within hundreds of 
   locations, which will lead to mis-configured the Nginx conf and make your
   service down.
2. It is hard to make sure your rewrite rules are well written.

## Demand Analysis

1. It help user debug nginx conf: find the location a request matched, or point
   out the url after rewrite
2. Can be used in online environment and develop environment, and no need to 
   build different binary:
   1. When used in online environment, it make no difference to business request
   2. When used in develop environment, it can debug the nginx configuration 
      without other dependency, espeically proxy scene
3. It is easy to be integrate into an auto test platform for large scale 
   configuration management.
4. Not to modify Nginx's official open source code for the convenience of adapting
   to all version of Nginx.


## Use Case View


## Logical View


## Development View


## Process View
