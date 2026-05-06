#include "PillIdentificationCache.h"
#include "../../Database/Connection.h"

namespace medibridge::services::pdma {

std::optional<Json::Value>
PillIdentificationCache::get_by_item_code(const std::string& item_code)
{
    // TODO (영역 B 분담):
    //   SELECT * FROM pill_identification WHERE item_code = ?
    //   파라미터 바인딩 필수
    return std::nullopt;
}

Json::Value
PillIdentificationCache::search_by_keys(const std::string& engraving,
                                        const std::string& shape,
                                        const std::string& color)
{
    // TODO: SELECT * FROM pill_identification
    //   WHERE engraving_front LIKE ? AND shape = ? AND color_front = ?
    return Json::Value(Json::arrayValue);
}

} // namespace medibridge::services::pdma
