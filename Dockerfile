FROM ubuntu:20.04
RUN DEBIAN_FRONTEND=noninteractive \
    apt-get update && \
    apt-get -y upgrade
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install nginx python3-pip python3 python3-wsgiproxy
RUN pip install --upgrade pip
#RUN pip install uwsgi web.py supervisor
RUN pip install web.py supervisor
RUN useradd -ms /bin/bash openinverter
RUN rm -f /etc/nginx/fastcgi.conf \
          /etc/nginx/fastcgi_params \
          /etc/nginx/snippets/fastcgi-php.conf \
          /etc/nginx/snippets/snakeoil.conf
COPY docker/supervisord.conf /usr/local/etc/supervisord.conf
COPY docker/wsgi.ini /etc/uwsgi/wsgi.ini
COPY docker/nginx.conf /etc/nginx/nginx.conf
COPY docker/app /home/openinverter/app
EXPOSE 5000
ENTRYPOINT ["/usr/local/bin/supervisord"]



