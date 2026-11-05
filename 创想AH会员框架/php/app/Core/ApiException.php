<?php
// 业务异常：控制器捕获后转成统一 JSON 错误响应

namespace CxSoft;

class ApiException extends \Exception
{
    /** @var string 接口错误码 */
    public $apiCode;
    /** @var string 面向用户的文案 */
    public $userMsg;
    /** @var array 附带数据 */
    public $data;

    public function __construct($apiCode, $msg = null, $data = [], \Throwable $previous = null)
    {
        $this->apiCode = (string) $apiCode;
        $this->userMsg = Code::msg($apiCode, $msg);
        $this->data    = (array) $data;
        parent::__construct(Code::describe($apiCode)[0], 0, $previous);
    }
}
