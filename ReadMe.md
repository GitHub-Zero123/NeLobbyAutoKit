# NeLobbyAutoKit.exe
适用于联机大厅项目开发的自动化便捷自测工具

## 配置
TARGET_CONFIG.json 用于声明目标参数
```python
// 该文件放置在exe同级以便读取解析
{
    "cookie": "...",              // 可在开发者平台f12调试模式->网络 随便点击一个MOD查看可以捕获到me字头的请求 其中包含你的账号cookie
    "modId": "...",               // MOD_ID或联机大厅项目ID
    "targetPath": "D:/xxx/xxxx",  // 项目路径
    // 打包忽略文件 若当前工具放置在项目中 请确保忽略自身
    "ignore": [
        "*.exe",
        "TARGET_CONFIG.json",
        "*/TARGET_CONFIG.json",
        "*/.mcs/*",
        "*/.git/*",
        "*/.gitignore",
        "*/studio.json",
        "*/work.mcscfg"
    ]

    // 不常用参数
    // "ignoreLog": true,    // 默认true, 输出忽略文件日志。
    // "outZipPath": "",    // 默认空，若设置具体路径（如D:/xxx/a.zip），将产生具体压缩文件，否则只在内存中处理。
}
```

## 运行
在满足配置条件后双击即可启动 将自动压缩打包项目并发布自测 若出现异常/安全执行完毕将弹出Toast提示窗 (win10+)

## 目录问题
联机大厅的压缩文件需要额外套一个`顶级文件夹`，因此targetPath需要注意。

## 注意
自动化打包压缩可能会忽略空文件夹 请确保entities/textures不为空(如新建一个__init__.py)在里面