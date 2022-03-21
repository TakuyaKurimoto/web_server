const express = require('express')
const app = express()
const port = 3000
const bodyParser = require('body-parser')
app.use(bodyParser.urlencoded({
    extended: true
}));
app.use(bodyParser.json());
app.get('/', (req, res) => {
  console.log('Hello World!')
  res.send('Hello World!')
})

app.post('/', (req, res) => {
  console.log(req.body)
  res.send('Post!')
})

app.listen(port, () => {
  console.log(`Example App listening on port ${port}`)
})
