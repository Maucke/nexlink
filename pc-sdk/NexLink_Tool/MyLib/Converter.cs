using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;

namespace NexLink_Tool
{
    public class HexConverter : IValueConverter
    {
        // 从数据转换为显示的十六进制字符串
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is int intValue)
            {
                return intValue.ToString("X2"); // X 表示以十六进制显示
            }
            return value;
        }

        // 从用户输入的十六进制字符串转换回数据类型
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            string hexString = value as string;
            if (!string.IsNullOrEmpty(hexString))
            {
                // 如果是十六进制格式，则解析为整数
                if (int.TryParse(hexString, NumberStyles.HexNumber, culture, out int result))
                {
                    return result;
                }
            }
            return value;
        }
    }
    public class WidthAdjusterConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is double actualWidth)
            {
                return actualWidth - 20;
            }
            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value;
        }
    }

}
