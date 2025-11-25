// Simple example demonstrating the validation capability

#include "cafPdmObject.h"
#include "cafPdmValidationCapability.h"
#include "cafPdmField.h"

#include <iostream>

class ExampleObject : public caf::PdmObject
{
public:
    ExampleObject()
    {
        CAF_PDM_InitObject("Example Object", "", "", "");
        
        // Add validation capability with callback to member function
        CAF_PDM_InitValidation( validateExample );
        
        CAF_PDM_InitField(&m_startValue, "StartValue", 0.0, "Start Value");
        CAF_PDM_InitField(&m_endValue, "EndValue", 100.0, "End Value");
        CAF_PDM_InitField(&m_name, "Name", QString(), "Name");
    }

private:
    // Custom validation method
    caf::ValidationResult validateExample( const QString& configName ) const
    {
        caf::ValidationResult result;
        
        // Cross-field validation
        if ( m_startValue > m_endValue )
        {
            result.status = caf::ValidationStatus::Error;
            result.fieldErrors["StartValue"] = "Start value must be less than end value";
            result.fieldErrors["EndValue"] = "End value must be greater than start value";
            result.message = "Invalid value range";
        }
        
        if ( m_name.value().isEmpty() )
        {
            result.status = std::max(result.status, caf::ValidationStatus::Warning);
            result.fieldErrors["Name"] = "Name should be specified";
        }
        
        return result;
    }

    caf::PdmField<double>  m_startValue;
    caf::PdmField<double>  m_endValue;
    caf::PdmField<QString> m_name;
};

int main()
{
    // Test 1: Valid object
    ExampleObject obj1;
    auto result1 = obj1.validate("UI");
    std::cout << "Test 1 (valid): " << (result1.status == caf::ValidationStatus::Valid ? "PASS" : "FAIL") << std::endl;
    
    // Test 2: Object with warning (empty name)
    ExampleObject obj2;
    obj2.fields()[2]->setValueFromUi(QVariant("")); // Set name to empty
    auto result2 = obj2.validate("UI");
    std::cout << "Test 2 (warning): " << (result2.status == caf::ValidationStatus::Warning ? "PASS" : "FAIL") << std::endl;
    
    // Test 3: Object with validation capability shows validation works
    std::cout << "Test 3 (capability): " << (caf::PdmValidationCapability::validateObject(&obj1, "UI").status == caf::ValidationStatus::Valid ? "PASS" : "FAIL") << std::endl;
    
    return 0;
}