const { defineConfig } = require('@vue/cli-service')
module.exports = defineConfig({
  transpileDependencies: true,
  devServer: {
    port: 8080,
    proxy: {
      '/api': {
        target: 'http://localhost:8081',
        changeOrigin: true,
        // pathRewrite: {'^/api': ''} // 如果后端接口不带 /api 前缀，可以启用这行
      }
    }
  }
})
