# NeLobbyAutoKit.exe
适用于联机大厅项目开发的自动化便捷自测工具

## 配置
TARGET_CONFIG.json 用于声明目标参数
```python
// 该文件放置在exe同级以便读取解析
{
    "cookie": "...",         // 可在开发者平台f12调试模式->网络 随便点击一个mod查看可以捕获到me字头的请求 其中包含你的账号cookie
    "modId": "...",              // MOD_ID或联机大厅项目ID
    "targetPath": "D:/xxx/xxxx",    // 项目路径
    // 打包忽略文件 若当前工具放置在项目中 请确保忽略自身
    "ignore": [
        "*.exe",
        "TARGET_CONFIG.json"
    ]
}
```

## 运行
在满足配置条件后双击即可启动 将自动压缩打包项目并发布自测 若出现异常/安全执行完毕将弹出Toast提示窗 (win10+)