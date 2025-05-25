// generate_embedded_files.js
const fs = require('fs');
const path = require('path');

// 函数：将文件名转换为合法的C++标识符
function sanitizeIdentifier(name) {
    return name.replace(/[^a-zA-Z0-9_]/g, '_').replace(/^(?=\d)/, '_');
}

// 主函数
function main(inputDir, outputFile) {
    fs.readdir(inputDir, (err, files) => {
        if (err) {
            console.error(`无法读取目录: ${inputDir}`);
            process.exit(1);
        }

        const embeddedFiles = [];

        files.forEach((filename) => {
            const filepath = path.join(inputDir, filename);
            if (fs.statSync(filepath).isFile()) {
                const content = fs.readFileSync(filepath);
                const hexContent = Array.from(content)
                    .map(byte => `0x${byte.toString(16).padStart(2, '0')}`)
                    .join(', ');
                const identifier = sanitizeIdentifier(filename);
                embeddedFiles.push({ filename, identifier, hexContent });
            }
        });

        let cppContent = '// Auto-generated embedded files\n';
        cppContent += '#include <cstddef>\n';
        cppContent += '#include <map>\n';
        cppContent += '#include <string>\n\n';
        cppContent += 'struct EmbeddedFile {\n';
        cppContent += '    const unsigned char* data;\n';
        cppContent += '    std::size_t size;\n';
        cppContent += '};\n\n';

        embeddedFiles.forEach(file => {
            cppContent += `const unsigned char ${file.identifier}_data[] = { ${file.hexContent} };\n`;
        });



        cppContent += 'std::map<std::string, EmbeddedFile> embedded_files = {\n';

        embeddedFiles.forEach(file => {
            cppContent += `    { "${file.filename}", { ${file.identifier}_data, sizeof(${file.identifier}_data) } },\n`;
        });

        cppContent += '};\n\n';


        fs.writeFile(outputFile, cppContent, 'utf8', (err) => {
            if (err) {
                console.error(`无法写入文件: ${outputFile}`);
                process.exit(1);
            }
            console.log(`成功生成嵌入文件: ${outputFile}`);
        });


    });
}

// 获取命令行参数
if (process.argv.length !== 4) {
    console.error('Usage: node generate_embedded_files.js <input_directory> <output_file.cpp>');
    process.exit(1);
}

const inputDir = process.argv[2];
const outputFile = process.argv[3];

main(inputDir, outputFile);