const express = require('express')
const app = express()
const port = 3000
const bodyParser = require('body-parser')
app.use(bodyParser.urlencoded({
    extended: true
}));
app.use(bodyParser.json());

const hoge = { a: 12123212322222222222222222222222222222222222, b: 343222222222222222222222222222222222222222222222222222222323 }
const resp = []
for (i = 0; i < 100000; i++){
  resp.push(hoge)
}
resp.push({a:1})
app.get('/', (req, res) => {
  console.log('Hello World!')
  res.send("hoge")
})

app.post('/', (req, res) => {
  //console.log(req.body)
  res.send('Post!')
})

app.listen(port, () => {
  console.log(`Example App listening on port ${port}`)
})
