const glob = require('glob');
const fs = require('fs');

glob(`./src/**/*.h`, (err, files) => {
  var result = `#include "enums.h"\n`;
  var header = '';
  header += "#ifndef INC_ENUMS\n"
  header += "#define INC_ENUMS\n"
  header += `#include "core/types.h"\n`;
  files.map((file) => {
    const content = fs.readFileSync(file).toString()
    content.replace(/enum (.*?)\s*\{([^}+]+)\}/g, (m, c, b) => {
      header += `// defined in ${file}\n`
      header += `char* get_${c}_name (uint32_t v);\n`
      result += `char* get_${c}_name (uint32_t v) {\n`
      result += 'switch (v) {\n'
      var value = 0;
      b.trim().replace(/\/\*.*?\*\//g, '').replace(/\/\/[^\n]+\n?/g, '').trim().split(/\s*,\s*/g).map((pairs) => {
        var [key, v] = pairs.split(/\s*=\s*/);
        if (!key) return;
        if (v) {
          const hex = v.match(/0x([A-F0-9]+)/i)
          if (hex) v = parseInt(hex[1], 16)
          else v = parseInt(v);
        } else {
          v = value;
        }
        result += `case ${v}: return "${key}";\n`
        
        value = v + 1
      })
      result += `default: return "Unknown";\n`
      result += '}\n';
      result += '};\n\n'
    })
  })
  header += "#endif"
  fs.writeFileSync('src/enums.h', header);
  fs.writeFileSync('src/enums.c', result);
  console.log(result)
})