<?php

/**
 * @file
 * Contains workflow\includes\Field\WorkflowDefaultWidget.
 */

/**
 * Plugin implementation of the 'workflow_default' widget.
 * @todo D8: Replace "extends WorkflowD7WidgetBase" by "extends WidgetBase"
 *           or perhaps by "extends OptionsWidgetBase" from Options module.
 *
 * @FieldWidget(
 *   id = "workflow_default",
 *   label = @Translation("Workflow"),
 *   field_types = {
 *     "workflow"
 *   },
 *   settings = {
 *     "name_as_title" = 1
 *     "comment" = 1
 *   }
 * )
 */
class WorkflowDefaultWidget extends WorkflowD7Base { // D8: extends WidgetBase {
  /*
   * Function, that gets replaced by the 'annotations' in D8. (@see comments above this class)
   */
  public static function settings() {
    return array(
      'workflow' => array(
        'label' => t('Workflow'),
        'field types' => array('workflow'),
        'settings' => array(
            'name_as_title' => 1,
            'comment' => 1,
          ),
      ),
    );
  }

  /**
   * {@inheritdoc}
   * The Widget Instance has no settings. To have a uniform UX, all settings are done on the Field level.
   */
  public function settingsForm(array $form, array &$form_state, $has_data) {
    $element = array();
    return $element;
  }

  /**
   * {@inheritdoc}
   * @todo D8: change "array $items" to "FieldInterface $items"
   */
  public function formElement(array $items, $delta, array $element, $langcode, array &$form, array &$form_state) {
    $field_name = $this->field['field_name'];
    $entity = $element['#entity'];
    $entity_type = 'node';
    if (!$entity) {
      // If no entity given, do not show a form. E.g., on the field settings page.
      return $element;
    }

    $settings_title_as_name = ($this->field['settings']['widget']['name_as_title'] && $this->instance['widget']['settings']['name_as_title']);
    $settings_schedule = $this->field['settings']['widget']['schedule'];
    $settings_schedule_timezone = $this->field['settings']['widget']['schedule_timezone'];
    // Show comment, when Field ánd Instance allow this.
    $settings_comment = ($this->field['settings']['widget']['comment'] && $this->instance['widget']['settings']['comment']) ? 'textarea' : 'hidden';

    $wid = $this->field['settings']['wid'];
    $workflow = new Workflow($wid);
    $name = t($workflow->name);

    // We need to decide here, which is the entity to act upon: are we working with a comment, or a node?
    // So, bluntly override "$entity = $element['#entity'];" with "$entity = $form['#node']".
    $entity = $form['#node'];
    $entity_type = 'node'; // comments are only supported on nodes.

    // @todo: Get the current sid for node, comment, preview.
    if (count($items)) {
      // A normal Node edit.
      $sid = _workflow_get_sid_by_items($items);
      $state = new WorkflowState($sid);
    }
    else {
      // Node add or Comment add.
      $items = field_get_items($entity_type, $entity, $field_name);
      if ($items) {
        $sid = _workflow_get_sid_by_items($items);
        $state = new WorkflowState($sid);
      }
      else {
        // Node add page: No valid sid is given, so get the first state.
        $state = $workflow->getFirstState($entity);
        $sid = $state->sid;
      }
    }

    $options = $state->getOptions($entity);

    // Get the scheduling info. This influences the current sid.
    $scheduled = '0';
    $timestamp = REQUEST_TIME;
    $comment = NULL;
    if ($settings_schedule == TRUE && isset($entity->nid)) {
      // Read scheduled information.
      // Technically you could have more than one scheduled, but this will only add the soonest one.
      foreach (workflow_get_workflow_scheduled_transition_by_nid($entity->nid) as $schedule_info) {
        $sid = $schedule_info->sid;
        $scheduled = '1';
        $timestamp = $schedule_info->scheduled;
        $comment = $schedule_info->comment;
        break;
      }
    }

    // Stop if user has no new target state(s) to choose.
    if (!workflow_show_form($sid, $workflow, $options)) {
      return $element;
    }

    $element['workflow'] = array(
      '#type' => 'fieldset',
      '#title' => $name,
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#weight' => 10,
      );

    // Save the current value of the node in the form.
    $element['workflow']['#node'] = $entity;

//dpm(array_pop($options));
//dpm(array_pop($options));
    if (count($options) == 1) {
    // Add the State widget/formatter
    // @todo: add real formatter, instead.
    // @todo: TEST THIS USE CASE.
      // There is no need to show the single choice.
      // A form choice would be an array with the key of the state.
      $state = key($options);
      $element['workflow'][$name] = array(
        '#type' => 'value',
        '#value' => array($state => $state),
        );
    }
    else {
      $element['workflow'] = array(
        '#type' => 'container',
        '#attributes' => array('class' => array('workflow-form-container')),
      );

//      $element['workflow'][$name] = array(
      $element['workflow']['workflow_options'] = array(
        '#type' => $this->field['settings']['widget']['options'],
        '#title' => $settings_title_as_name ? t('Change !name state', array('!name' => $name)) : '',
        '#options' => $options,
//        '#name' => $name,
//        '#parents' => array('workflow'),
        '#default_value' => $sid,
        );
    }

    // Display scheduling form, but only if a node is being edited and user has
    // permission. State change cannot be scheduled at node creation because
    // that leaves the node in the (creation) state.
    if ($settings_schedule == TRUE
        && !(arg(0) == 'node' && arg(1) == 'add') 
        && user_access('schedule workflow transitions')) {

    $element['workflow']['workflow_scheduled'] = array(
      '#type' => 'radios',
      '#title' => t('Schedule'),
      '#options' => array(
          '0' => t('Immediately'),
          '1' => t('Schedule for state change'),
        ),
      '#default_value' => $scheduled,
      );

    // @todo: now that we work on an element instead of the form itself, #states doesn't work anymore: 
    // @todo: the scheduling date is not hidden anymore.
    $element['workflow']['workflow_scheduled_date_time'] = array(
      '#type' => 'fieldset',
      '#title' => t('At'),
      '#prefix' => '<div style="margin-left: 1em;">',
      '#suffix' => '</div>',
      '#states' => array(
        'visible' => array(':input[name="workflow_scheduled"]' => array('value' => 1)),
        'invisible' => array(':input[name="workflow_scheduled"]' => array('value' => 0)),
        ),
      );

      $element['workflow']['workflow_scheduled_date_time']['workflow_scheduled_date'] = array(
        '#type' => 'date',
        '#default_value' => array(
          'day'   => date('j', $timestamp),
          'month' => date('n', $timestamp),
          'year'  => date('Y', $timestamp),
        ),
      );

      $hours = format_date($timestamp, 'custom', 'H:i');
      $element['workflow']['workflow_scheduled_date_time']['workflow_scheduled_hour'] = array(
        '#type' => 'textfield',
        '#description' => t('Please enter a time in 24 hour (eg. HH:MM) format.
          If no time is included, the default will be midnight on the specified date.
          The current time is: @time', array('@time' => $hours)),
        '#default_value' => $scheduled ?
          (isset($form_state['values']['workflow_scheduled_hour']) ?
            $form_state['values']['workflow_scheduled_hour'] : $hours) : '00:00',
        );

      global $user;
      if (variable_get('configurable_timezones', 1) && $user->uid && drupal_strlen($user->timezone)) {
        $timezone = $user->timezone;
      }
      else {
        $timezone = variable_get('date_default_timezone', 0);
      }

      $timezones = drupal_map_assoc(timezone_identifiers_list());

      $element['workflow']['workflow_scheduled_date_time']['workflow_scheduled_timezone'] = array(
        '#type' => $settings_schedule_timezone ? 'select' : 'hidden',
        '#options' => $timezones,
        '#title' => t('Time zone'),
        '#default_value' => array($timezone => $timezone),
        );
    }

    $element['workflow']['workflow_comment'] = array(
      '#type' => $settings_comment,
      '#title' => t('Workflow comment'),
      '#description' => t('A comment to put in the workflow log.'),
      '#default_value' => $comment,
      '#rows' => 2,
      );

    return $element;
  }

  /*
   * Implements workflow_transition() -> submit() 
   * Overrides submit(array $form, array &$form_state).
   * Contains 2 extra parameters for D7
   * @param array $items
   *   The value of the field.
   * @param array $force
   *   A boolean. TRUE if all access must be overridden, e.g., for Rules.
   *
   * This is called from function _workflowfield_form_submit($form, &$form_state)
   * It is a replacement of function workflow_transition($node, $new_sid, $force, $field)
   * It performs the following actions; 
   * - save a scheduled action
   * - update history
   * - restore the normal $items for the field.
   * @todo: remove update of {node_form} table. (separate task, because it has features, too)
   */
  public function submit(array $form, array &$form_state, array &$items = array(), $force = FALSE) {
    global $user;

    $entity = $form['#node']; // @todo: deprecated in D8.
    $entity_type = isset($form['#node_type']) ? $form['#node_type'] : ''; // custom setting.

    $field = $this->field;
    $old_sid = workflow_node_current_state($entity, $field);
    $new_sid = $items[0]['workflow']['workflow_options'];

    // The scheduling options may not be set in formElement(), e.g, on 'node add' pages.
    $scheduled = isset($items[0]['workflow']['workflow_scheduled']) ? $items[0]['workflow']['workflow_scheduled'] : 0;
    $schedule = isset($items[0]['workflow']['workflow_scheduled_date_time']) ? (object) $items[0]['workflow']['workflow_scheduled_date_time'] : NULL;
    $comment = $items[0]['workflow']['workflow_comment'];

    if ($force) {
      // @todo: move this function to class.
      $options = workflow_get_other_states_by_sid($new_sid);
    }
    else {
      $state = new WorkflowState($new_sid);
      $options = workflow_field_choices($entity, $force, $state);
    }

    // Only execute if the new state is a valid choice.
    if ($new_sid && array_key_exists($new_sid, $options)) {
      if (!$scheduled) {
        // It's an immediate change. Do the transition.
        // - validate option; add hook to let other modules change comment.
        // - add to history; add to watchdog
        // return the new value of the sid. (execution may fail.)
        $new_sid = workflow_execute_transition($entity, $new_sid, $comment, $force = FALSE, $field, $old_sid);

        // Restore the default values for Workflow Field.
        $items = array();
        $items[0]['value'] = $new_sid;
      }
      else {
        // Schedule the time to change the state.
        if ($schedule->workflow_scheduled_date['day'] < 10) {
          $schedule->workflow_scheduled_date['day'] = '0' .
          $schedule->workflow_scheduled_date['day'];
        }
        if ($schedule->workflow_scheduled_date['month'] < 10) {
          $schedule->workflow_scheduled_date['month'] = '0' .
          $schedule->workflow_scheduled_date['month'];
        }
        if (!isset($schedule->workflow_scheduled_hour)) {
          $schedule->workflow_scheduled_hour = '00:00';
        }

        $scheduled_date_time =
            $schedule->workflow_scheduled_date['year']
          . $schedule->workflow_scheduled_date['month']
          . $schedule->workflow_scheduled_date['day']
          . ' '
          . $schedule->workflow_scheduled_hour
          . ' '
          . $schedule->workflow_scheduled_timezone
          ;

        if ($scheduled_date_time = strtotime($scheduled_date_time)) {
          // Clear previous entries and insert.
          $data = array(
            'nid' => $entity->nid,
            'old_sid' => $old_sid,
            'sid' => $new_sid,
            'uid' => $user->uid,
            'scheduled' => $scheduled_date_time,
            'comment' => $comment,
            );

          workflow_insert_workflow_scheduled_transition($data);
          // Get name of state.
          if ($state = workflow_get_workflow_states_by_sid($new_sid)) {
            $t_args = array(
                '@node_title' => $entity->title,
                '%state_name' => t($state->state),
                '%scheduled_date' => format_date($scheduled_date_time),
                );
            watchdog('workflow', '@node_title scheduled for state change to %state_name on %scheduled_date', $t_args,
              WATCHDOG_NOTICE, l('view', 'node/' . $entity->nid . '/workflow'));
            drupal_set_message(t('@node_title is scheduled for state change to %state_name on %scheduled_date',
              $t_args));
          }
        }

        // Restore the default values for Workflow Field.
        $items = array();
        $items[0]['value'] = $old_sid;
      }
    }
  }

  public function errorElement(array $element, ConstraintViolationInterface $violation, array $form, array &$form_state) {
  }

  public function settingsSummary() {
  }

  /**
   * Returns the array of options for the widget.
   *
   * @return array
   *   The array of options for the widget.
   */
//  protected function getOptions() {
//    // @todo: move workflow_field_options here, into Widget::getOptions().

//    @todo: remove: These are the testdata from list_test.
//        This is listed here because it hints to the use of 'Phases', 
//        just like Commerce module uses State ('Phase') vs. Status (Workflow State).
//    $values = array(
//      'Group 1' => array(
//        0 => 'Zero',
//      ),
//      2 => 'One',
//      'Group 2' => array(
//        2 => 'Some <script>dangerous</script> & unescaped <strong>markup</strong>',
//      ),
//    );
//
//    return $values;
//  }
}
