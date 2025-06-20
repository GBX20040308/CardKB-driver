#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

#define POLL_INTERVAL_MS 20
#define REG_KEY_CODE     0x5f

struct cardkb {
    struct i2c_client *client;
    struct input_dev *input;
    struct delayed_work work;
    u16 last_keycode;
    bool last_shift;
};

// ⌨️ 自定义键值映射表（大小写键码不同）
static const struct {
    u8 raw_code;     // 从硬件读取的字节值
    u16 keycode;     // Linux 逻辑键位
    bool shift;      // 是否需要上报 Shift
} cardkb_keymap[] = {
    // 标准按键（无需 Shift）
    { 0x1B, KEY_ESC, false },      // Esc 键
    { 0x31, KEY_1, false },        // 数字 1
    { 0x32, KEY_2, false },        // 数字 2
    { 0x33, KEY_3, false },        // 数字 3
    { 0x34, KEY_4, false },        // 数字 4
    { 0x35, KEY_5, false },        // 数字 5
    { 0x36, KEY_6, false },        // 数字 6
    { 0x37, KEY_7, false },        // 数字 7
    { 0x38, KEY_8, false },        // 数字 8
    { 0x39, KEY_9, false },        // 数字 9
    { 0x30, KEY_0, false },        // 数字 0
    { 0x08, KEY_BACKSPACE, false },// 退格键
    { 0x09, KEY_TAB, false },      // Tab 键
    { 0x61, KEY_A, false },        // 小写 'a'
    { 0x62, KEY_B, false },        // 小写 'b'
    { 0x63, KEY_C, false },        // 小写 'c'
    { 0x64, KEY_D, false },        // 小写 'd'
    { 0x65, KEY_E, false },        // 小写 'e'
    { 0x66, KEY_F, false },        // 小写 'f'
    { 0x67, KEY_G, false },        // 小写 'g'
    { 0x68, KEY_H, false },        // 小写 'h'
    { 0x69, KEY_I, false },        // 小写 'i'
    { 0x6A, KEY_J, false },        // 小写 'j'
    { 0x6B, KEY_K, false },        // 小写 'k'
    { 0x6C, KEY_L, false },        // 小写 'l'
    { 0x6D, KEY_M, false },        // 小写 'm'
    { 0x6E, KEY_N, false },        // 小写 'n'
    { 0x6F, KEY_O, false },        // 小写 'o'
    { 0x70, KEY_P, false },        // 小写 'p'
    { 0x71, KEY_Q, false },        // 小写 'q'
    { 0x72, KEY_R, false },        // 小写 'r'
    { 0x73, KEY_S, false },        // 小写 's'
    { 0x74, KEY_T, false },        // 小写 't'
    { 0x75, KEY_U, false },        // 小写 'u'
    { 0x76, KEY_V, false },        // 小写 'v'
    { 0x77, KEY_W, false },        // 小写 'w'
    { 0x78, KEY_X, false },        // 小写 'x'
    { 0x79, KEY_Y, false },        // 小写 'y'
    { 0x7A, KEY_Z, false },        // 小写 'z'
    { 0xB5, KEY_UP, false },       // 上箭头
    { 0xB6, KEY_DOWN, false },     // 下箭头
    { 0xB4, KEY_LEFT, false },     // 左箭头
    { 0xB7, KEY_RIGHT, false },    // 右箭头
    { 0x0D, KEY_ENTER, false },    // 回车
    { 0x20, KEY_SPACE, false },    // 空格
    // 大写字母（需要 Shift）
    { 0x41, KEY_A, true },         // 大写 'A'
    { 0x42, KEY_B, true },         // 大写 'B'
    { 0x43, KEY_C, true },         // 大写 'C'
    { 0x44, KEY_D, true },         // 大写 'D'
    { 0x45, KEY_E, true },         // 大写 'E'
    { 0x46, KEY_F, true },         // 大写 'F'
    { 0x47, KEY_G, true },         // 大写 'G'
    { 0x48, KEY_H, true },         // 大写 'H'
    { 0x49, KEY_I, true },         // 大写 'I'
    { 0x4A, KEY_J, true },         // 大写 'J'
    { 0x4B, KEY_K, true },         // 大写 'K'
    { 0x4C, KEY_L, true },         // 大写 'L'
    { 0x4D, KEY_M, true },         // 大写 'M'
    { 0x4E, KEY_N, true },         // 大写 'N'
    { 0x4F, KEY_O, true },         // 大写 'O'
    { 0x50, KEY_P, true },         // 大写 'P'
    { 0x51, KEY_Q, true },         // 大写 'Q'
    { 0x52, KEY_R, true },         // 大写 'R'
    { 0x53, KEY_S, true },         // 大写 'S'
    { 0x54, KEY_T, true },         // 大写 'T'
    { 0x55, KEY_U, true },         // 大写 'U'
    { 0x56, KEY_V, true },         // 大写 'V'
    { 0x57, KEY_W, true },         // 大写 'W'
    { 0x58, KEY_X, true },         // 大写 'X'
    { 0x59, KEY_Y, true },         // 大写 'Y'
    { 0x5A, KEY_Z, true },         // 大写 'Z'
    // 符号（部分需要 Shift）
    { 0x21, KEY_1, true },         // '!' (Shift + 1)
    { 0x40, KEY_2, true },         // '@' (Shift + 2)
    { 0x23, KEY_3, true },         // '#' (Shift + 3)
    { 0x24, KEY_4, true },         // '$' (Shift + 4)
    { 0x25, KEY_5, true },         // '%' (Shift + 5)
    { 0x5E, KEY_6, true },         // '^' (Shift + 6)
    { 0x26, KEY_7, true },         // '&' (Shift + 7)
    { 0x2A, KEY_8, true },         // '*' (Shift + 8)
    { 0x28, KEY_9, true },         // '(' (Shift + 9)
    { 0x29, KEY_0, true },         // ')' (Shift + 0)
    { 0x7B, KEY_LEFTBRACE, true }, // '{' (Shift + [)
    { 0x7D, KEY_RIGHTBRACE, true },// '}' (Shift + ])
    { 0x5B, KEY_LEFTBRACE, false },// '['
    { 0x5D, KEY_RIGHTBRACE, false },// ']'
    { 0x2F, KEY_SLASH, false },    // '/' （注意：原始代码此处缺少逗号）
    { 0x5C, KEY_BACKSLASH, false },// '\'
    { 0x7C, KEY_BACKSLASH, true }, // '|' (Shift + \)
    { 0x7E, KEY_GRAVE, true },     // '~' (Shift + `)
    { 0x27, KEY_APOSTROPHE, false },// '''
    { 0x22, KEY_APOSTROPHE, true },// '"' (Shift + ')
    { 0x3B, KEY_SEMICOLON, false },// ';'
    { 0x3A, KEY_SEMICOLON, true }, // ':' (Shift + ;)
    { 0x60, KEY_GRAVE, false },    // '`'
    { 0x2B, KEY_EQUAL, true },     // '+' (Shift + =)
    { 0x2D, KEY_MINUS, true },     // '-' (Shift + -)
    { 0x5F, KEY_MINUS, false },    // '_'
    { 0x3D, KEY_EQUAL, false },    // '='
    { 0x3F, KEY_SLASH, true },     // '?' (Shift + /)
    { 0x3C, KEY_COMMA, true },     // '<' (Shift + ,)
    { 0x3E, KEY_DOT, true },       // '>' (Shift + .)
    { 0x2C, KEY_COMMA, false },    // ','
    { 0x2E, KEY_DOT, false },      // '.'
    // 功能键区（fn）在原始代码中未完成
};

static const int keymap_size = ARRAY_SIZE(cardkb_keymap);

static void cardkb_poll(struct work_struct *work)
{
    struct cardkb *kb = container_of(work, struct cardkb, work.work);
    struct i2c_client *client = kb->client;
    struct input_dev *input = kb->input;
    int val = i2c_smbus_read_byte_data(client, REG_KEY_CODE);
    int i;

    if (val < 0) {
        return;
    }
    /*
    // 特殊处理：0xA8 触发 Ctrl+C
    if (val == 0xA8) {
        input_report_key(input, KEY_LEFTCTRL, 1);
        input_sync(input);
        //msleep(10);

        input_report_key(input, KEY_C, 1);
        input_sync(input);
        msleep(20);

        input_report_key(input, KEY_C, 0);
        input_sync(input);
       // msleep(10);

        input_report_key(input, KEY_LEFTCTRL, 0);
        input_sync(input);

        kb->last_keycode = 0;
        kb->last_shift = false;
        schedule_delayed_work(&kb->work, msecs_to_jiffies(POLL_INTERVAL_MS));
        return;
    }
    */
    if (val == 0) {
        // 按键释放逻辑
        if (kb->last_keycode) {
            if (kb->last_shift)
                input_report_key(input, KEY_LEFTSHIFT, 0);
            input_report_key(input, kb->last_keycode, 0);
            input_sync(input);
            kb->last_keycode = 0;
            kb->last_shift = false;
        }
    } else {
        // 查找映射项
        for (i = 0; i < keymap_size; i++) {
            if (cardkb_keymap[i].raw_code == val) {
                // 如果上次有键，先释放
                if (kb->last_keycode &&
                    (kb->last_keycode != cardkb_keymap[i].keycode ||
                     kb->last_shift != cardkb_keymap[i].shift)) {
                    if (kb->last_shift)
                        input_report_key(input, KEY_LEFTSHIFT, 0);
                    input_report_key(input, kb->last_keycode, 0);
                }

                // 上报 Shift（如果需要）
                if (cardkb_keymap[i].shift)
                    input_report_key(input, KEY_LEFTSHIFT, 1);

                // 上报主键
                input_report_key(input, cardkb_keymap[i].keycode, 1);
                input_sync(input);

                kb->last_keycode = cardkb_keymap[i].keycode;
                kb->last_shift = cardkb_keymap[i].shift;
                break;
            }
        }
    }

    schedule_delayed_work(&kb->work, msecs_to_jiffies(POLL_INTERVAL_MS));
}

static int cardkb_probe(struct i2c_client *client)
{
    struct cardkb *kb;
    struct input_dev *input;
    int err, i;

    kb = devm_kzalloc(&client->dev, sizeof(*kb), GFP_KERNEL);
    if (!kb)
        return -ENOMEM;

    input = devm_input_allocate_device(&client->dev);
    if (!input)
        return -ENOMEM;

    kb->client = client;
    kb->input = input;
    kb->last_keycode = 0;
    kb->last_shift = false;

    input->name = "CardKB I2C Keyboard";
    input->id.bustype = BUS_I2C;

    __set_bit(EV_KEY, input->evbit);
    __set_bit(EV_REP, input->evbit);

    // 设置支持的按键
    for (i = 0; i < keymap_size; i++) {
        __set_bit(cardkb_keymap[i].keycode, input->keybit);
        if (cardkb_keymap[i].shift)
            __set_bit(KEY_LEFTSHIFT, input->keybit);
    }

    err = input_register_device(input);
    if (err) {
        return err;
    }

    i2c_set_clientdata(client, kb);
    INIT_DELAYED_WORK(&kb->work, cardkb_poll);
    schedule_delayed_work(&kb->work, msecs_to_jiffies(POLL_INTERVAL_MS));

    return 0;
}

static void cardkb_remove(struct i2c_client *client)
{
    struct cardkb *kb = i2c_get_clientdata(client);
    cancel_delayed_work_sync(&kb->work);
}

static const struct of_device_id cardkb_of_match[] = {
    { .compatible = "openai,cardkb" },
    { }
};
MODULE_DEVICE_TABLE(of, cardkb_of_match);

static struct i2c_driver cardkb_driver = {
    .driver = {
        .name = "cardkb",
        .of_match_table = cardkb_of_match,
    },
    .probe  = cardkb_probe,
    .remove = cardkb_remove,
};
module_i2c_driver(cardkb_driver);

MODULE_AUTHOR("你的名字");
MODULE_DESCRIPTION("I2C CardKB Keyboard Driver (with Shift Mapping)");
MODULE_LICENSE("GPL");

