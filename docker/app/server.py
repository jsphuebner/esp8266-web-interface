import web

urls = (
    '/cmd', 'cmd',
    '/(.*)', 'hello'
)
app = web.application(urls, globals())

class cmd:
    def GET(self):
        return '{}'

class hello:
    def GET(self, name):
        if not name:
            name = 'World'
        return 'Hello, ' + name + '!'

if __name__ == "__main__":
    app.run()
