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

```
       request     -----
 O   -----------> |     |
-|-               |Nginx|
/ \  <----------- |     |
       conf info   -----
```

Especially, when Nginx is used as a proxy

Online mode: Whether our module working or not, it should not impact on Nginx's normal function.

```
                   -----
 O     request    |     |   request    ----------
-|-  -----------> |Nginx| ----------> | Upstream |
/ \               |     |              ----------
                   -----
```
or
```
       request     -----
 O   -----------> |     |   request    ----------
-|-               |Nginx| ----------> | Upstream |
/ \  <----------- |     |              ----------
       conf info   -----
```

Development mode:
```
       request     -----
 O   -----------> |     |   request     ----------
-|-               |Nginx| -----X-----> | Upstream |
/ \  <----------- |     |               ----------
       conf info   -----
```
## Logical View

The whole process of module must be:
1. Find the correct config for current request matched
2. Save the matching result
3. Output conf message

NGX_HTTP_ACCESS_PHASE is the best choice for ngx_http_conf_debug_module
because:
1. It may interrupt the whole request
2. It will not influence normal request processing

### Find Config

Open source Nginx already has "NGX_HTTP_FIND_CONFIG_PHASE", which is designe
to find 

### Storage

According to Nginx architecture, variable is a common ways for module
comunication. For the convenience for decoupling the output ways, variable
is a good choice.

In order to manager easily, there must be a variable name scheme to:
1. Identify the variable providing module.
2. Add more varible as ngx_http_conf_debug_module supporting more and more
   config debuging needs.

Variable name starts with: $conf_debug_, follow by config name and property.
For example:
* $conf_debug_location_mode
* $conf_debug_location_name
* ...


### Output

ngx_http_conf_debug_module is used in to scene:
1. Interrupt normal request
2. Not Interrupt normal request

Considering:
1. Debug message need to places in http header becase http body is defined
   by upstream and has no extendibility.
2. Reusing current Nginx response editing ability is the best choice.

User must use "add_header" directive to add debug message in response header.
Header add by user is recommended to start with: X-Conf-Debug-.
For example:
```
add_header X-Conf-Debug-Location-Mode $conf_debug_location_mode always;
```

"always" is needed becase in "Not interrupt normal request" scene, upstream 
server may reseponse various of http status code, which may make add header
failed

By the way, in "Not interrupt notmal request" scene, we strongly recommend
user to add some network isolation(for example: iptables) to prevent nginx
sends the offline request to online server.

## Process View

## Development View


## Physical View
