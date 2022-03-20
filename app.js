const express = require('express')
const app = express()
const port = 3000

app.get('/', (req, res) => {
  console.log('Hello World!')
  res.send('Hello World!')
})

app.post('/', (req, res) => {
  console.log('Post!')
  res.send('Post!')
})

app.listen(port, () => {
  console.log(`Example App listening on port ${port}`)
})
