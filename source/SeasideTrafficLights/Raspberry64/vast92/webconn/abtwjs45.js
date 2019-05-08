// The userErrorRoutine variable can be set to override the default error display algorithm
// An 'alert' prompter is displayed by default
var userErrorRoutine

// Verify that the passed value is numeric 
function isNumeric ( inputValue ) {
	oneDecimal = false
	inputStr = inputValue.toString()
	for ( var i = 0 ; i < inputStr.length ; i++ ) {
		var oneChar = inputStr.charAt(i)
		if ( i == 0 && oneChar == "-" ) {
			continue
		}
		if ( oneChar == "." && !oneDecimal ) {
			oneDecimal = true
			continue
		}
		if ( oneChar < "0" || oneChar > "9" ) {
			return false
		}
	}
	return true
}


// Verify that the passed field contains data
function isEmpty ( inputValue ) {

	if ( inputValue == null || inputValue == "" )  {
		return true
	}
		return false
}


// Verify that the value in the passed input field is numeric.  Issue an error if the data is not numeric
function vaValidateNumeric ( inputField ) {

	if ( !(isEmpty ( inputField.value ))) {
		if ( isNumeric ( inputField.value )) {
			return true } }

	vaReportError ( "Enter a number into the field" )
	inputField.focus()
	return false
	} 

// Display the error using the 'alert' method or execute a userErrorRoutine if
// available
function vaReportError ( errorString ) {
	if ( userErrorRoutine != null ) {
		eval ( userErrorRoutine )
	} 
	else {
		alert ( errorString ) }
	}

// Verify that the user input is a value between min and max
function vaValidateNumericRange ( inputField, min, max ) {
	var lowNum , highNum
	if ( vaValidateNumeric ( inputField ) ) {
		lowNum = parseFloat ( min )
		highNum = parseFloat ( max )
		if ( vaValidateRange ( inputField , lowNum, highNum )) {
			return true }
		inputField.focus()
		return false
	} } 

// Return true if value falls between min and max
function vaValidateRange ( inputField , min , max ) {
	if ( vaValidateMin ( inputField , min )) {
		if ( vaValidateMax ( inputField , max )) { 
			return true } } 
	inputField.focus()
	return false
	}

// Verify that the passed value is greater than min
function vaValidateMin ( inputField , min ) {
	if ( inputField.value >= min ) {
		return true }
	vaReportError ( "Enter a value greater than or equal to " + min )
	inputField.focus()
	return false } 

// Verify that the passed value is less than max
function vaValidateMax ( inputField , max ) {
	if ( inputField.value <= max ) {
		return true }
	vaReportError ( "Enter a value less than or equal to " + max )
	inputField.focus()
	return false } 

// Convert the text in the passed field to upper case and replace its 'value'
// with the upper cased string
function vaConvertToUpperCase ( inputField ) {	
	if ( !(isEmpty (inputField.value ))) {	
		inputField.value = inputField.value.toUpperCase() } }

// Convert the text in the passed field to lower case and replace its 'value'
// with the lower cased string
function vaConvertToLowerCase ( inputField ) {	
	if ( !(isEmpty (inputField.value ))) {	
		inputField.value = inputField.value.toLowerCase() } }

// Verify that the user has entered at least minChars
function vaValidateMinChars ( inputField , minChars ) {
	if ( inputField.value.length >= minChars ) {
		return true }
	vaReportError ( "Field requires at least " + minChars + " characters" )
	inputField.focus()
	return false }
