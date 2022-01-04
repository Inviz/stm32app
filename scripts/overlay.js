const fs = require('fs');
const inflection = require('inflection')

var base = fs.readFileSync(process.argv[3]).toString();

const overlays = process.argv.slice(4).map((o) => fs.readFileSync(o).toString());


const values = {};

var count = 0;
overlays.map((overlay) => {
  overlay.replace(/(x\d{4}_.*?) = \{([\s\S]*?)    \}/g, (m, name, content) => {
    content.replace(/    \.([^\s]+) = ([^\n]+?)\n/g, (m, k, v) => {
      if (!values[name]) values[name] = {};
      if (k == 'highestSub_indexSupported') return;
      count++;
      values[name][k] = v;
    })
  })
})

const output = base.replace(/(x\d{4}_.*?) = \{([\s\S]*?)    \}/g, (m, name, content) => {
  return m.replace(/    \.([^\s]+) = ([^\n]+?)\n/g, (m, k, v) => {
    if (!values[name] || !values[name][k]) return m;
    return `    .${k} = ${values[name][k]}\n`;
  })
})

console.log(`${process.argv[2]} compiled with ${count} overlayed values`)

fs.writeFileSync(process.argv[2], `#ifndef OD_DEFINITION
${output}
#endif`);