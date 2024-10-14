using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;


public class I2CErrorParser
{
    // 定义错误代码
    private const uint HAL_I2C_ERROR_NONE = 0x00000000U;    // 无错误
    private const uint HAL_I2C_ERROR_BERR = 0x00000001U;    // BERR 错误
    private const uint HAL_I2C_ERROR_ARLO = 0x00000002U;    // ARLO 错误
    private const uint HAL_I2C_ERROR_AF = 0x00000004U;      // AF 错误
    private const uint HAL_I2C_ERROR_OVR = 0x00000008U;     // OVR 错误
    private const uint HAL_I2C_ERROR_DMA = 0x00000010U;     // DMA 传输错误
    private const uint HAL_I2C_ERROR_TIMEOUT = 0x00000020U; // 超时错误
    private const uint HAL_I2C_ERROR_SIZE = 0x00000040U;    // 尺寸管理错误
    private const uint HAL_I2C_ERROR_DMA_PARAM = 0x00000080U; // DMA 参数错误
    private const uint HAL_I2C_WRONG_START = 0x00000200U;   // 错误的开始错误

    public static string ParseI2CError(uint errorCode)
    {
        if (errorCode == HAL_I2C_ERROR_NONE)
            return "无错误";

        string errorMessage = "发生错误：";

        if ((errorCode & HAL_I2C_ERROR_BERR) != 0)
            errorMessage += "总线错误; ";
        if ((errorCode & HAL_I2C_ERROR_ARLO) != 0)
            errorMessage += "仲裁丢失; ";
        if ((errorCode & HAL_I2C_ERROR_AF) != 0)
            errorMessage += "无应答; ";
        if ((errorCode & HAL_I2C_ERROR_OVR) != 0)
            errorMessage += "溢出错误; ";
        if ((errorCode & HAL_I2C_ERROR_DMA) != 0)
            errorMessage += "DMA 传输错误; ";
        if ((errorCode & HAL_I2C_ERROR_TIMEOUT) != 0)
            errorMessage += "超时错误; ";
        if ((errorCode & HAL_I2C_ERROR_SIZE) != 0)
            errorMessage += "尺寸管理错误; ";
        if ((errorCode & HAL_I2C_ERROR_DMA_PARAM) != 0)
            errorMessage += "DMA 参数错误; ";
        if ((errorCode & HAL_I2C_WRONG_START) != 0)
            errorMessage += "错误的开始错误; ";

        return errorMessage.TrimEnd(';', ' '); // 去掉最后的分号和空格
    }
}

// 使用示例
//class Program
//{
//    static void Main(string[] args)
//    {
//        uint errorCode = 0x00000005U; // 假设发生了 BERR 和 AF 错误
//        string result = I2CErrorParser.ParseI2CError(errorCode);
//        Console.WriteLine(result); // 输出: 发生错误：总线错误; 无应答
//    }
//}