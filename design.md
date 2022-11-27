# Nginx Debug module

version: 1.0.0

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

### Nginx location config storage and find location config

For most scene consideration, we don't take nested location and @name location
into account.

Parsing and saving location config is impletmented in **ngx_http_core_location**
function.
location infomation store in clcf(ngx_http_core_loc_conf_t), core properties are 
as followed:
1. name
2. exact_match
3. noregex
4. regex
5. named(for @name scene, not cared)

As Nginx doesn't have a data stucture to store location mode information, we need
to find it through reduction. 
Nginx has 5 location mode, and :
1. =, exact_match == 1, noregex == 0, regex == NULL
2. ~, exact_match == 0, noregex == 0, regex != NULL,
   pcre_fullinfo(regex->regex->code, PCRE_INFO_OPTIONS) & PCRE_CASELESS == 0
3. ~*, exact_match == 0, noregex == 0, regex != NULL
   pcre_fullinfo(regex->regex->code, PCRE_INFO_OPTIONS) & PCRE_CASELESS == 1
4. ^~, exact_match == 0, noregex == 1, regex == NULL
5. <none>, exact_match == 0, noregex == 0, regex == NULL

Since $proxy_host variable are not available until NGX_HTTP_CONTENT_PHASE, we
need to get ngx_http_proxy_module configuration, which is not visible for 
ngx_http_conf_debug_module. Therefore, we need to redefine a data structure to
get proxy_host's value ahead.
**Keey in mind**, redefined data structure must keep same as definition in 
ngx_http_proxy_module.  

**The resule above base on assumption**: macro NGX_HTTP_CASELESS_FILESYSTEM hasn't
defined(if defined, Nginx will take ~ as caseless)

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

If a request match a normal prefix mode location, such as "location / {}",
"blank" will be set as variable's value because some middleware will remove the
header when its value is "".

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
