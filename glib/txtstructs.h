
struct MsgData { long  code;
                 char *string;
               };

struct LanguageInfo { enum Language   language;
                      struct MsgData *errors;
                      struct MsgData *strings;
                    };

