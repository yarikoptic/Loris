<?
require_once "NDB_Form.class.inc";
require_once('RequiredIf.inc');

/**
 * Base Form class
 * @package main
 */
class NDB_Reliability extends NDB_Form
{
    /**
     * Class name
     * @var    string
     * @access private
     */
    var $reliability_center_id;


    /**
     * Generates a new form instance and runs the appropriate method
     *
     * @param  string      Identifies the form
     * @param  string      Identifies the page to show
     * @param  string      Variables passed to form
     * @return void
     * @throws PEAR_Error
     * @access public
     */
    function &factory($name, $page, $identifier, $reliability_center_id)
    {
        // check that the form class exists
        if (class_exists("NDB_Reliability_$name")) {
            $class = "NDB_Reliability_$name";
        } else {
            return PEAR::raiseError("The form class ($name) is not defined.");
        }

        // create a form instance
        $obj = new $class;

        // set the local variables
        $obj->form = new HTML_QuickForm('test_form');
        $obj->name = $name;
        $obj->page = $page;
        $obj->identifier = $identifier;
        $obj->reliability_center_id = $reliability_center_id;
        
        $access = $obj->_hasAccess();
        if (PEAR::isError($access)) {
            return PEAR::raiseError("NDB_Form::factory() access: ".$access->getMessage());
        }

        // check that user has access
        if ($access == false) {
            return PEAR::raiseError("You do not have access to this page.");
        }

 	    if (method_exists($obj, $page)) {
	        $success = $obj->$page();
            if (PEAR::isError($success)) {
                return PEAR::raiseError("NDB_Form::factory() page: ".$success->getMessage());
            }
            $obj->template = $page;
	    }
 	    elseif (method_exists($obj, $name)) {
	        $success = $obj->$name();
            if (PEAR::isError($success)) {
                return PEAR::raiseError("NDB_Form::factory() name: ".$success->getMessage());
            }
            $obj->template = $name;
	    }
        else {
            return PEAR::raiseError("Form does not exist: $name $page");
        }

        return $obj;
    }



    /**
     * Displays the form
     *
     * @return void
     * @access public
     */
    function display()
    {
        if($_REQUEST['blind'] == '1') {
            $this->form->addElement('hidden', 'blind', $_REQUEST['blind']);
        }
        if(!$this->form->isFrozen()) {
            // hidden values
            $this->form->addElement('hidden', 'test_name', $this->name);
            $this->form->addElement('hidden', 'subtest', $this->page);
            $this->form->addElement('hidden', 'identifier', $this->identifier);
         }


        // get the defaults
        $localDefaults = $this->_getDefaults();
        if (PEAR::isError($localDefaults)) {
            return PEAR::raiseError("NDB_Form::display(): ".$localDefaults->getMessage());
        }
        if(!is_array($localDefaults)) { $localDefaults = array(); }
        // set the quickform object defaults
        $this->form->setDefaults(array_merge($this->defaults, $localDefaults));

	    // trim all values
    	$this->form->applyFilter('__ALL__', 'trim');

        // display the HTML_Quickform object
        $smarty = new Smarty_neurodb;
        $renderer =& new HTML_QuickForm_Renderer_ArraySmarty($smarty);
        $this->form->accept($renderer);
        $smarty->assign('form', $renderer->toArray());
        $smarty->assign($this->tpl_data);
        $html = $smarty->fetch("form_$this->template.tpl");
        return $html;
    }

    /**
     * Saves the form
     *
     * Attempts to validate the form (using the defined rules) and
     * saves the validated data into the database
     *
     * @return void
     * @throws PEAR_Error
     * @access public
     */
    function save()
    {
        if ($this->form->validate()) {
            $success = $this->form->process(array(&$this, "_save"), false);
            if (PEAR::isError($success)) {
                return PEAR::raiseError("NDB_Form::save(): ".$success->getMessage());
            }
        }
    }
    

    /**
     * Preprocesses the array of values to be saved into the database
     *
     * @param  array    the array of values ready to be passed to a Database::replace call as the set array
     * @return void
     * @access private
     */
    function _save($values)
    {
        unset($values['test_name'], $values['subtest'], $values['identifier'], $values['fire_away'], $values['blind']);

        // clear any fields starting with __
        foreach (array_keys($values) AS $key) {
            if (strpos($key, '__') === 0) {
                unset($values[$key]);
            }
        }

        $success = $this->_process($values);
        if (PEAR::isError($success)) {
            return PEAR::raiseError("NDB_Form::_save(): ".$success->getMessage());
        }
    }

    function addDateElement($name, $label){
         $group [] = $this->form->createElement("date", $name . "_date", null, $this->dateOptions);
         $this->dateTimeFields[] = $name . "_date"; //add to array of dates and times.
         $group [] = $this->form->createElement("select", $name . "_date_status", null, array(NULL=>"", 'not_answered'=>"Not Answered"));
         $this->form->addGroup($group, $name . "_date_group", $label, $this->_GUIDelimiter, FALSE);
         unset($group);
         $this->XINRegisterRule($name . "_date", array($name . "_date_status{@}=={@}"), "A Date, or Not Answered is required.", $name . "_date_group");

     }
     function XINRegisterRule($elname, $rules, $message="", $group=""){
         if(!is_array($rules)){
             $rules_array[]=$rules;
         } else {
             $rules_array=$rules;
         }
         $this->XINRules[$elname]['message']=$message;
         $this->XINRules[$elname]['group']=$group;

         foreach($rules_array AS $rule){
             $this->XINRules[$elname]['rules'][]=$rule;
         }
         return true;
     }
     function addTextElement($field, $label, $rules=array(), $rule_message='This field is required.', $refusals=array(''=>'', 'not_answered'=>'Not Answered')){
         $group[]=&$this->form->createElement("text", $field);
         $group[]=&$this->form->createElement("select", $field."_status", "", $refusals);
         $this->form->addGroup($group, $field.'_group', $label, null, false);
         unset($group);
         $rules_array=array_merge(array($field.'_status{@}=={@}'), $rules);
         $this->XINRegisterRule($field, $rules_array, $rule_message, $field.'_group');
     }
     
     /**
      * Wrapper to create a text area field, with an accompanying status field.
      * @param    string    $field    The database field in which the response will be stored
      * @param    string    $label    The question text to display
      * @param    array    $rules    {@} delimited rules
      * @param    string    $rule_message    rule message to display upon rule violation
      */
      function addTextAreaElement($field, $label, $rules=array(), $rule_message='This field is required.'){
          $group[]=&$this->form->createElement("textarea", $field, null, array('cols'=>25, 'rows'=>4));
          $group[]=&$this->form->createElement("select", $field."_status", "", array(NULL=>'', 'not_answered'=>'Not Answered'));
          $this->form->addGroup($group, $field.'_group', $label, null, false);
          unset($group);
          $rules_array=array_merge(array($field.'_status{@}=={@}'),$rules);
          $this->XINRegisterRule($field, $rules_array, $rule_message, $field.'_group');
      }

      function addLargeTextAreaElement($field, $label = "&nbsp;&nbsp;&nbsp;&nbsp;Describe:", $rules=array(), $rule_message='This field must be completed or marked as not answered.') {
         return;  //removed all free form comment text boxes.
         $group[]=&$this->form->createElement("textarea", $field, null, array('cols'=>60, 'rows'=>6));
         $group[]=&$this->form->createElement("select", $field."_status", "", array(NULL=>'', 'not_answered'=>'Not Answered'));
         $this->form->addGroup($group, $field.'_group', $label, null, false);
         unset($group);
         $rules_array=array_merge(array($field.'_status{@}=={@}'),$rules);
         $this->XINRegisterRule($field, $rules_array, $rule_message, $field.'_group');
     }
     
     function _addMetadataFields()
       {
           $config =& NDB_Config::singleton();
           $dateOptions = array(
                                'language' => 'en',
                                'format'   => 'YMd',
                                'minYear'  => $config->getSetting('startYear'),
                                'maxYear'  => $config->getSetting('endYear'),
                                'addEmptyOption' => true,
                                'emptyOptionValue' => null
                                );

           $this->form->addElement('date', 'Date_taken', 'Date of Administration', $dateOptions);

           $examiners = $this->_getExaminerNames();
           $this->form->addElement('select', 'Examiner', 'Reliability Coder', $examiners);

           //$this->form->addElement('text', 'Examiner', 'Examiner');

           //$this->form->addGroupRule('Date_taken', 'Date of Administration is required', 'required');

           $this->form->registerRule('checkdate', 'callback', '_checkDate');
           //$this->form->registerRule('requiredIf', null, 'HTML_QuickForm_Rule_RequiredIf', 'RequiredIf.inc');
           
           $this->form->addElement('select', 'invalid', '<font color="red">Mark as invalid</font>', array("no" => "No", "yes" => "YES"));
           $this->form->addRule('invalid', 'Invalid flag is required.', 'required');
           
           $this->form->addRule('Date_taken', 'Date of Administration is invalid', 'checkdate');

           //$this->form->addRule(array('invalid', 'Examiner'), 'Examiner is required,', 'requiredIf', array('operator'=>'==', 'value'=>'no'));
           //$this->setRequired('invalid');
           
       }

       function _getExaminerNames()
        {
            $db =& Database::singleton();
            $user =& User::singleton();
            $centerID = $user->getCenterID();
            //$centerID = $db->selectOne("SELECT session.CenterID FROM session, flag WHERE session.ID=flag.SessionID and flag.CommentID='".$this->identifier."'");

            // UofA is a special case--they never enter their own data. So the available examiners are either the ones
            // at the logged in user's site, or UofA
            $db->select("SELECT examinerID, full_name FROM examiners WHERE centerID IN ('$centerID', 6) ORDER BY full_name", $results);
            if($db->isError($results)) {
                return $this->raiseError("Could not get examiner names: ".$results->getMessage());
            }

            $examiners = array(''=>'');
            if(is_array($results) && !empty($results)) {
                foreach($results AS $row) {
                    $examiners[$row['examinerID']] = $row['full_name'];
                }
            }
            return $examiners;
        }

        function _getDatabaseDate($formDateValue){
             if(!empty($formDateValue)){
                 if(count($formDateValue)==2){
                     $databaseValue = "$formDateValue[H]:$formDateValue[i]";  //handle time (HH:MM:SS)
                 } else {
                     $databaseValue = "$formDateValue[Y]-$formDateValue[M]-$formDateValue[d]";  //handle date (YYYY/MM/DD)
                 }
             }
             return $databaseValue;
         }
     
}
?>